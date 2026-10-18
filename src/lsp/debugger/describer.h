/**
 * aaltitoad - a verification engine for tick tock automata models
   Copyright (C) 2026 Asger Gitz-Johansen

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
#ifndef AALTITOAD_LSP_DEBUGGER_DESCRIBER_H
#define AALTITOAD_LSP_DEBUGGER_DESCRIBER_H
#include <parser/graphedit/network.h>
#include <nlohmann/json.hpp>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace aaltitoad::debugger {
    // The type of a value, for the user ("int", "float", "bool", "string" or "clock")
    auto type_of(const expr::symbol_value_t& value) -> std::string;
    // A value as json (clocks as their time units)
    auto value_to_json(const expr::symbol_value_t& value) -> nlohmann::json;

    // A variable of a state: where it is (global or an instance) and its name there
    struct variable_name_t {
        std::string instance{};
        std::string name{};
    };

    // Describes the states and the steps of a network in the json of the debug protocol of graphedit
    // (docs/debugging.md of graphedit), for the debugger and for the traces of queries: the instances with their
    // active locations and their variables, and the edges of steps (the elements of graphedit models that they involve)
    // with labels for the user.
    //
    // Instances are named after their scope (e.g. "Main.lamp(1)"); public and external variables are the global variables
    // of a state, private variables are the variables of their instance.
    class describer {
    public:
        // How many edges the label of a step names (a tick of a large network takes many)
        static constexpr std::size_t max_label_edges = 3;

        // The network must outlive the describer
        explicit describer(const graphedit::network_t& network);

        // The instances and the variables of a state of the network: {"instances": [...], "variables": {...}}
        auto state(const ntta_t& state) const -> nlohmann::json;
        // Where a symbol is: the instance and the name of a private variable, or the global name of the others
        auto variable_of(const std::string& symbol) const -> variable_name_t;
        // Whether a symbol is a private variable of an instance
        auto is_private(const std::string& symbol) const -> bool;

        // The edges that a step takes (by their identifiers in the network), ordered by their instances
        auto edges_of(const std::vector<std::string>& edges) const -> std::vector<const graphedit::instance_edge_t*>;
        // The elements of graphedit models that a step involves, per instance: [{"instance", "model", "ids"}]
        auto elements_of(const std::vector<std::string>& edges) const -> nlohmann::json;
        // A label for a step: the edges that it takes, leaving out those that every choice of the state takes (common),
        // with their guards (or updates) if detailed
        auto label(const std::vector<std::string>& edges, const std::set<const graphedit::instance_edge_t*>& common = {},
                   bool detailed = false) const -> std::string;
        // A label for a change of the environment (e.g. "button.1 = true, setting = 4")
        auto input_label(const expr::symbol_table_t& changes) const -> std::string;
        // A location of a model, for the user: its nickname, or what kind of location it is
        auto location_name(const std::string& model, const std::string& location) const -> std::string;

    private:
        const graphedit::network_t& network_;
        // private symbol -> its instance and name
        std::map<std::string, variable_name_t> private_symbols_{};
    };
}

#endif //AALTITOAD_LSP_DEBUGGER_DESCRIBER_H
