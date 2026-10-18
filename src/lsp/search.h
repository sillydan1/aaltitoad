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
#include "debugger/describer.h"
#include <verification/forward_reachability.h>
#include <nlohmann/json.hpp>
#include <chrono>
#include <vector>

namespace aaltitoad::lsp {
    // Answers a query about a network with the forward reachability search of aaltitoad's verifier: it searches the
    // states that the network reaches by its ticks and by the changes of its environment that the interesting tocker
    // finds, until a state decides the query or every reachable state was searched. The search is taken a state at a time in slices (see run), so that the server keeps handling messages
    // meanwhile, and stops without an answer after a number of states.
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
        // How many states were searched, and how many wait to be searched
        auto states() const -> std::size_t;
        auto waiting() const -> std::size_t;
        // The result of verify/run, once the answer is known: {"status", "message", "statistics", "trace"}
        auto result() const -> nlohmann::json;

    private:
        // The answer of the searcher, once its search ended
        void conclude();
        void finish(status_t status, std::string message);
        // The step from a state of a trace to the next one: a tick, or a change of the environment
        auto step_between(const ntta_t& from, const ntta_t& to) const -> nlohmann::json;
        auto trace() const -> nlohmann::json;

        graphedit::network_t network_;
        // NOTE: after the network, which it describes
        debugger::describer describer_;
        query_t query_;
        std::size_t max_states_;
        forward_reachability_searcher searcher_{};
        bool started_{false};
        status_t status_{status_t::searching};
        std::string message_{};
        // the states from the initial state to the state that decided the query
        std::vector<ntta_t> trace_{};
    };
}

#endif //AALTITOAD_LSP_SEARCH_H
