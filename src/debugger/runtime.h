/**
 * aaltitoad - a verification engine for tick tock automata models
   Copyright (C) 2023 Asger Gitz-Johansen

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef AALTITOAD_DEBUGGER_RUNTIME_H
#define AALTITOAD_DEBUGGER_RUNTIME_H
#include <parser/graphedit/network.h>
#include <nlohmann/json.hpp>
#include <chrono>
#include <deque>
#include <map>
#include <optional>
#include <random>
#include <stdexcept>

// Debugging networks of graphedit models with the debug protocol of graphedit (docs/debugging.md of graphedit): a
// runtime takes the steps of a network (ticks), reports its states and stops at breakpoints.
namespace aaltitoad::debugger {
    // The error codes of the debug protocol
    inline constexpr int method_not_found = -32601;
    inline constexpr int invalid_params = -32602;
    inline constexpr int not_now = -32000;

    // A request that is refused, with the error code and the message for the user
    class request_error : public std::runtime_error {
    public:
        request_error(int code, const std::string& message) : std::runtime_error{message}, code{code} {}
        int code;
    };

    // A breakpoint of the debug protocol: on an element (a vertex or an edge of a model), on a condition, or both
    struct breakpoint_t {
        std::string id;
        // the model and the element (both empty for breakpoints that are only a condition)
        std::string model;
        std::string element;
        // only this instance counts (optional)
        std::string instance;
        std::string condition;
        // the first hit_count - 1 hits are ignored
        int hit_count{0};
        auto operator==(const breakpoint_t&) const -> bool = default;
    };
    // Throws request_error (invalid_params) if the json is not a breakpoint
    auto breakpoint_from_json(const nlohmann::json& json) -> breakpoint_t;

    struct breakpoint_status_t {
        std::string id;
        bool verified;
        std::string message;
    };
    auto to_json(const breakpoint_status_t& status) -> nlohmann::json;

    enum class stop_reason { entry, step, breakpoint, pause, stuck, error };
    auto to_string(stop_reason reason) -> std::string;

    // Why and where the runtime stopped
    struct stop_t {
        stop_reason reason;
        // the breakpoints that were hit
        std::vector<std::string> breakpoints{};
        // for the user
        std::string description{};
    };

    // How the runtime chooses its steps when it runs
    enum class policy_t { first, random };

    // The arguments of a debug configuration
    struct arguments_t {
        policy_t policy{policy_t::first};
        // the seed of the random choices (random if there is none)
        std::optional<unsigned int> seed{};
        // milliseconds between two steps when the runtime runs
        int delay{0};
        // how many states can be restored
        std::size_t history{1000};
    };
    // Throws request_error (invalid_params) if the arguments are invalid
    auto arguments_from_json(const nlohmann::json& json) -> arguments_t;

    // Runs a network of graphedit models: takes steps (ticks), reports states in the json of the debug protocol, stops
    // at breakpoints and changes and restores states.
    //
    // A step is a tick: a set of edges that are enabled and do not conflict, at most one per instance (ntta_t::tick);
    // the network has no tockers, so the environment only changes when its variables are set. Instances are named
    // after their scope (e.g. "Main.lamp(1)"); public and external variables are the global variables of a state, private
    // variables are the variables of their instance. Every variable is writable.
    class runtime {
    public:
        // How many choices a state lists (a tick may enable many combinations of edges)
        static constexpr std::size_t max_choices = 100;
        // How many edges the label of a step names (a tick of a large network takes many)
        static constexpr std::size_t max_label_edges = 3;

        // Throws std::invalid_argument if the network was not built
        runtime(graphedit::network_t network, arguments_t arguments);

        auto arguments() const -> const arguments_t&;
        auto network() const -> const graphedit::network_t&;
        // Replace the breakpoints. Hits are counted from the moment a breakpoint was set: breakpoints that stay the same
        // keep their hits.
        auto set_breakpoints(const std::vector<breakpoint_t>& breakpoints) -> std::vector<breakpoint_status_t>;
        // The current state (the json of the debug protocol). Reporting a state makes it restorable, unless it is a live
        // state (reported while the runtime runs, many times a second), which the history does not keep.
        auto state(bool live = false) -> nlohmann::json;
        // Take a step: the choice of the current state with the id, or (std::nullopt) a choice of the policy. Throws
        // request_error (invalid_params) if there is no such choice. Returns how the step ended: step, breakpoint, stuck
        // (no step is possible) or error (the state did not change).
        auto step(const std::optional<std::string>& choice) -> stop_t;
        // Take steps of the policy until the runtime stops (at a breakpoint, stuck or failing), at most max_steps and
        // not after the deadline. Returns the stop, or std::nullopt if it did not stop.
        auto run(std::size_t max_steps, std::chrono::steady_clock::time_point deadline) -> std::optional<stop_t>;
        // Change variables: {"x": 1, "Main.lamp(1):on": true}. Throws request_error (invalid_params) and changes nothing
        // if a variable does not exist or cannot take its value.
        void set_variables(const nlohmann::json& values);
        // Put an instance into a location: ["<location id>"]. Throws request_error (invalid_params) if there is no such
        // instance or location.
        void set_active(const std::string& instance, const nlohmann::json& active);
        // Return to a state that was reported. Throws request_error (invalid_params) if it cannot be restored.
        void restore(const std::string& state);

    private:
        struct snapshot_t {
            std::map<std::string, std::string> locations{};
            expr::symbol_table_t symbols{};
            expr::symbol_table_t external_symbols{};
        };
        struct active_breakpoint_t {
            breakpoint_t definition{};
            bool verified{false};
            std::string message{};
            // the locations, or the edges of the model (their ids), that the breakpoint is on
            std::set<std::string> locations{};
            std::set<std::string> edges{};
            // the condition, compiled in the scope of each instance that it concerns ("" for the global scope)
            std::map<std::string, expr::syntax_tree_t> conditions{};
            int hits{0};
            // whether the condition (of a breakpoint without element) held in the current state
            bool held{false};
        };
        // What a step did: instance -> the location that it entered, and instance -> the edges that it took
        struct step_effect_t {
            std::map<std::string, std::string> entered{};
            std::map<std::string, std::vector<const graphedit::edge_t*>> taken{};
        };
        // A variable of a state: where it is (global or an instance) and its name there
        struct variable_name_t {
            std::string instance{};
            std::string name{};
        };

        void changed();
        auto choices() -> const std::vector<ntta_t::state_change_t>&;
        auto take(std::size_t choice) -> stop_t;
        auto choose() -> std::size_t;
        // Compile a breakpoint: find what it is on and compile its condition. Returns why it cannot be compiled
        auto compile(active_breakpoint_t& breakpoint) -> std::optional<std::string>;
        // Find the location, or the edges of the template, that the element of a breakpoint is on. Returns why there are none
        auto locate(active_breakpoint_t& breakpoint) const -> std::optional<std::string>;
        // Compile the condition of a breakpoint in the scope of each instance (nullptr is the global scope). Returns why
        // it cannot be compiled
        auto compile_condition(active_breakpoint_t& breakpoint, const std::vector<const graphedit::instance_t*>& scopes) -> std::optional<std::string>;
        auto holds(const active_breakpoint_t& breakpoint, const std::string& scope) const -> bool;
        void update_held();
        // What a step did: the instances that entered locations, and the edges (of their templates) that they took
        auto effect_of(const ntta_t::state_change_t& change) const -> step_effect_t;
        // Where a breakpoint was hit by a step (for the user), if it was. Hits are not counted here
        auto hit(active_breakpoint_t& breakpoint, const step_effect_t& effect) -> std::optional<std::string>;
        // A breakpoint without an element is hit when its condition starts to hold
        auto condition_hit(active_breakpoint_t& breakpoint) const -> std::optional<std::string>;
        // A breakpoint on an element is hit when the instance enters its location or takes one of its edges
        auto element_hit(const active_breakpoint_t& breakpoint, const graphedit::instance_t& instance,
                         const step_effect_t& effect) const -> std::optional<std::string>;
        auto location_name(const std::string& model, const std::string& location) const -> std::string;
        // A label for a step: the edges that it takes, leaving out those that every choice of the state takes (common)
        auto label(const ntta_t::state_change_t& change, const std::set<const graphedit::instance_edge_t*>& common = {}) const -> std::string;
        // The edges of a step, ordered by their instances (parents before the instances that they instantiate)
        auto edges_of(const ntta_t::state_change_t& change) const -> std::vector<const graphedit::instance_edge_t*>;
        auto variable_of(const std::string& symbol) const -> variable_name_t;
        auto take_snapshot() const -> snapshot_t;
        void apply_snapshot(const snapshot_t& snapshot);

        graphedit::network_t network_;
        arguments_t arguments_;
        std::mt19937 random_;
        std::vector<active_breakpoint_t> breakpoints_{};
        // private symbol -> its instance and name
        std::map<std::string, variable_name_t> private_symbols_{};
        // the choices of the current state (computed when needed), or why they could not be computed
        std::optional<std::vector<ntta_t::state_change_t>> choices_{};
        std::string choices_error_{};
        // the id of the current state, once it was reported
        std::optional<std::string> state_id_{};
        int next_state_{0};
        std::map<std::string, snapshot_t> history_{};
        std::deque<std::string> history_order_{};
    };
}

#endif //AALTITOAD_DEBUGGER_RUNTIME_H
