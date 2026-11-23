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
#include "search.h"
#include <ntta/interesting_tocker.h>
#include <algorithm>
#include <stdexcept>

namespace aaltitoad::lsp {
    namespace {
        auto count(std::size_t n, const std::string& one, const std::string& many) -> std::string {
            return std::to_string(n) + " " + (n == 1 ? one : many);
        }

        // why a step could not be taken, for the user
        auto failure(const std::exception& e) -> std::string {
            if(dynamic_cast<const std::bad_variant_access*>(&e) != nullptr)
                return "an expression has a value of the wrong type (e.g. a guard that is not boolean)";
            return e.what();
        }

        auto value_of(const expr::symbol_value_t& value) -> const expr::underlying_symbol_value_t& {
            return static_cast<const expr::underlying_symbol_value_t&>(value);
        }
    }

    search::search(graphedit::network_t network, forward_reachability_searcher::query_solution_t query, std::size_t max_states)
            : network_{std::move(network)}, describer_{network_}, query_{std::move(query)}, max_states_{max_states} {
        if(!network_.automata)
            throw std::invalid_argument("aaltitoad could not build the network");
    }

    auto search::status() const -> status_t {
        return status_;
    }

    auto search::message() const -> const std::string& {
        return message_;
    }

    auto search::states() const -> std::size_t {
        return searcher_.passed();
    }

    auto search::waiting() const -> std::size_t {
        return searcher_.waiting();
    }

    auto search::run(std::chrono::steady_clock::time_point deadline) -> bool {
        try {
            if(!started_) {
                started_ = true;
                // the environment changes as the interesting tocker of aaltitoad's verifier finds
                auto initial = *network_.automata;
                initial.add_tocker(std::make_shared<interesting_tocker>());
                searcher_.start(initial, {query_});
            }
            while(status_ == status_t::searching) {
                if(searcher_.step())
                    conclude();
                else if(searcher_.passed() >= max_states_)
                    finish(status_t::unknown, "The search stopped after " + count(max_states_, "state", "states") +
                                              " (the option maxStates) without an answer.");
                else if(std::chrono::steady_clock::now() >= deadline)
                    // NOTE: between the slices, the server reports the progress (language_server::search_further)
                    break;
            }
        } catch(const std::exception& e) {
            finish(status_t::failed, "aaltitoad cannot take a step: " + failure(e));
        }
        return status_ != status_t::searching;
    }

    void search::conclude() {
        const auto& answer = searcher_.results().front();
        auto reachability = answer.kind == forward_reachability_searcher::query_kind_t::reachability;
        if(!answer.solution) {
            // every reachable state was searched
            auto all = count(searcher_.passed(), "state", "states");
            if(reachability)
                finish(status_t::violated, "No reachable state satisfies the predicate (" + all + " searched).");
            else
                finish(status_t::satisfied, "The predicate holds in all " + all + " that can be reached.");
            return;
        }
        for(auto it = std::optional{answer.solution.value()}; it.has_value(); it = it.value()->second.parent)
            trace_.push_back(it.value()->second.data);
        std::reverse(trace_.begin(), trace_.end());
        auto steps = trace_.size() - 1;
        auto when = steps == 0 ? std::string{"in the initial state"} : "in " + count(steps, "step", "steps");
        if(reachability)
            finish(status_t::satisfied, "A state that satisfies the predicate is reached " + when + ".");
        else
            finish(status_t::violated, "A state that violates the predicate is reached " + when + ".");
    }

    void search::finish(status_t status, std::string message) {
        if(status_ != status_t::searching)
            return;
        status_ = status;
        message_ = std::move(message);
    }

    auto search::step_between(const ntta_t& from, const ntta_t& to) const -> nlohmann::json {
        // NOTE: the searcher keeps the states of a trace, not the steps between them: a tick leads to the next state, or
        //       else the environment changed (the tick is preferred if both do)
        auto state = from;
        for(auto& change : state.tick())
            if(from + change == to)
                return {{"label", describer_.label(change.edges)}, {"kind", "step"},
                        {"elements", describer_.elements_of(change.edges)}};
        expr::symbol_table_t input{};
        for(auto& [name, value] : to.external_symbols)
            if(auto before = from.external_symbols.find(name);
               before == from.external_symbols.end() || value_of(before->second) != value_of(value))
                input[name] = value;
        return {{"label", describer_.input_label(input)}, {"kind", "input"}, {"elements", nlohmann::json::array()}};
    }

    auto search::trace() const -> nlohmann::json {
        auto steps = nlohmann::json::array();
        for(std::size_t i = 0; i < trace_.size(); i++) {
            nlohmann::json step{{"state", describer_.state(trace_[i])}};
            if(i > 0)
                step["step"] = step_between(trace_[i - 1], trace_[i]);
            steps.push_back(std::move(step));
        }
        return {{"steps", steps}};
    }

    auto search::result() const -> nlohmann::json {
        auto status = status_ == status_t::satisfied ? "satisfied" : status_ == status_t::violated ? "violated" : "unknown";
        nlohmann::json result{{"status", status}, {"message", message_},
                              {"statistics", {{"states", searcher_.passed()}, {"waiting", searcher_.waiting()}}}};
        if(!trace_.empty())
            result["trace"] = trace();
        return result;
    }
}
