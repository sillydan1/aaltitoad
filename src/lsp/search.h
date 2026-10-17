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
#ifndef AALTITOAD_LSP_SEARCH_H
#define AALTITOAD_LSP_SEARCH_H
#include "query.h"
#include <debugger/describer.h>
#include <nlohmann/json.hpp>
#include <chrono>
#include <deque>
#include <optional>
#include <unordered_map>
#include <vector>

namespace aaltitoad::lsp {
    // Answers a query about a network by forward reachability, as aaltitoad's verifier does: it searches the states that
    // the network reaches by its ticks and by the changes of its environment that the interesting tocker finds (values of
    // the external variables that make the guards of the current locations hold or not), until a state decides the query
    // (see query_t::decides) or every reachable state was searched. As in the verifier, the environment may also stay as
    // it is after a tick. The states are searched breadth first, so that the traces are as short as possible, and in
    // slices (see run), so that the server keeps handling messages meanwhile.
    class search {
    public:
        enum class status_t {
            searching,
            // the answers of verify/run (docs/verification.md of graphedit)
            satisfied,
            violated,
            unknown,
            // the network could not take a step (e.g. a guard cannot be evaluated): see message
            failed
        };

        // Search the states of the network (which must have been built) for the query, at most max_states of them
        search(graphedit::network_t network, query_t query, std::size_t max_states);
        search(const search&) = delete;
        search(search&&) = delete;
        auto operator=(const search&) -> search& = delete;
        auto operator=(search&&) -> search& = delete;
        ~search() = default;

        // Search until the answer is known or the deadline passed. Returns whether the answer is known.
        auto run(std::chrono::steady_clock::time_point deadline) -> bool;
        auto status() const -> status_t;
        // Why the search ended, for the user
        auto message() const -> const std::string&;
        // How many states were found, and how many of them were searched
        auto states() const -> std::size_t;
        auto searched() const -> std::size_t;
        // The result of verify/run, once the answer is known: {"status", "message", "statistics", "trace"}
        auto result() const -> nlohmann::json;

    private:
        // A state that the search found, and how: from which state, by which edges (a tick) or by which changes of the
        // environment (a tock); the first state has neither
        struct node_t {
            ntta_t state;
            std::optional<std::size_t> parent{};
            std::vector<std::string> edges{};
            std::optional<expr::symbol_table_t> input{};
        };

        // Add a state that was found (unless it was found before). Returns its index, or std::nullopt if it is not new.
        auto add(ntta_t state, std::optional<std::size_t> parent, std::vector<std::string> edges,
                 std::optional<expr::symbol_table_t> input) -> std::optional<std::size_t>;
        // Whether a state that was added decides the query (and ends the search)
        auto decides(std::size_t node) -> bool;
        // Add the states that the environment can change a state into (to be searched later). Returns whether the search
        // ended.
        auto add_inputs(std::size_t node, const std::vector<expr::symbol_table_t>& inputs) -> bool;
        // Search the successors of a state. Returns whether the search ended.
        auto expand(std::size_t node) -> bool;
        void finish(status_t status, std::string message, std::optional<std::size_t> decided = std::nullopt);
        // The number of steps from the first state to a state
        auto depth(std::size_t node) const -> std::size_t;
        auto trace(std::size_t node) const -> nlohmann::json;

        graphedit::network_t network_;
        // NOTE: after the network, which it describes
        debugger::describer describer_;
        query_t query_;
        std::size_t max_states_;
        std::deque<node_t> nodes_{};
        // hash -> the states with that hash
        std::unordered_map<std::size_t, std::vector<std::size_t>> known_{};
        std::deque<std::size_t> frontier_{};
        std::size_t searched_{0};
        std::size_t transitions_{0};
        bool started_{false};
        status_t status_{status_t::searching};
        std::string message_{};
        // the state that decided the query
        std::optional<std::size_t> decided_{};
    };
}

#endif //AALTITOAD_LSP_SEARCH_H
