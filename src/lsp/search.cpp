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
#include <hashcombine>
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
    }

    search::search(graphedit::network_t network, query_t query, std::size_t max_states)
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
        return nodes_.size();
    }

    auto search::searched() const -> std::size_t {
        return searched_;
    }

    auto search::run(std::chrono::steady_clock::time_point deadline) -> bool {
        try {
            if(!started_) {
                started_ = true;
                // the environment changes as the interesting tocker of aaltitoad's verifier finds
                auto initial = *network_.automata;
                initial.add_tocker(std::make_shared<interesting_tocker>());
                auto first = add(std::move(initial), std::nullopt, {}, std::nullopt);
                frontier_.push_back(first.value());
                if(decides(first.value()) || add_inputs(first.value(), nodes_[first.value()].state.tock()))
                    return true;
            }
            while(status_ == status_t::searching) {
                if(frontier_.empty()) {
                    auto all = count(nodes_.size(), "state", "states");
                    if(query_.kind() == query_t::kind_t::reachability)
                        finish(status_t::violated, "No reachable state satisfies the predicate (" + all + " searched).");
                    else
                        finish(status_t::satisfied, "The predicate holds in all " + all + " that can be reached.");
                    break;
                }
                auto node = frontier_.front();
                frontier_.pop_front();
                if(expand(node) || std::chrono::steady_clock::now() >= deadline)
                    break;
            }
        } catch(const std::exception& e) {
            finish(status_t::failed, "aaltitoad cannot take a step: " + failure(e));
        }
        return status_ != status_t::searching;
    }

    auto search::add(ntta_t state, std::optional<std::size_t> parent, std::vector<std::string> edges,
                     std::optional<expr::symbol_table_t> input) -> std::optional<std::size_t> {
        // NOTE: the hash of aaltitoad leaves the external variables out, which tell the changes of the environment apart
        auto hash = ya::hash_combine(std::hash<ntta_t>{}(state), state.external_symbols);
        auto& same = known_[hash];
        if(std::any_of(same.begin(), same.end(), [this, &state](std::size_t other) { return nodes_[other].state == state; }))
            return std::nullopt;
        if(nodes_.size() >= max_states_) {
            finish(status_t::unknown, "The search stopped after " + count(max_states_, "state", "states") +
                                      " (the option maxStates) without an answer.");
            return std::nullopt;
        }
        nodes_.push_back({std::move(state), parent, std::move(edges), std::move(input)});
        same.push_back(nodes_.size() - 1);
        return nodes_.size() - 1;
    }

    auto search::decides(std::size_t node) -> bool {
        if(!query_.decides(nodes_[node].state))
            return false;
        auto steps = depth(node);
        auto when = steps == 0 ? std::string{"in the initial state"} : "in " + count(steps, "step", "steps");
        if(query_.kind() == query_t::kind_t::reachability)
            finish(status_t::satisfied, "A state that satisfies the predicate is reached " + when + ".", node);
        else
            finish(status_t::violated, "A state that violates the predicate is reached " + when + ".", node);
        return true;
    }

    auto search::add_inputs(std::size_t node, const std::vector<expr::symbol_table_t>& inputs) -> bool {
        for(const auto& input : inputs) {
            transitions_++;
            auto changed = add(nodes_[node].state + input, node, {}, input);
            if(status_ != status_t::searching)
                return true;
            if(!changed)
                continue;
            frontier_.push_back(changed.value());
            if(decides(changed.value()))
                return true;
        }
        return false;
    }

    auto search::expand(std::size_t node) -> bool {
        searched_++;
        // NOTE: after a tick, the environment may change or stay as it is: a state after a tick is searched further as
        //       it is, and changed by the environment
        for(auto& change : nodes_[node].state.tick()) {
            transitions_++;
            auto next = add(nodes_[node].state + change, node, change.edges, std::nullopt);
            if(status_ != status_t::searching)
                return true;
            if(!next)
                continue;
            if(decides(next.value()))
                return true;
            frontier_.push_back(next.value());
            if(add_inputs(next.value(), nodes_[next.value()].state.tock()))
                return true;
        }
        return false;
    }

    void search::finish(status_t status, std::string message, std::optional<std::size_t> decided) {
        if(status_ != status_t::searching)
            return;
        status_ = status;
        message_ = std::move(message);
        decided_ = decided;
    }

    auto search::depth(std::size_t node) const -> std::size_t {
        std::size_t result = 0;
        for(auto parent = nodes_[node].parent; parent.has_value(); parent = nodes_[parent.value()].parent)
            result++;
        return result;
    }

    auto search::trace(std::size_t node) const -> nlohmann::json {
        std::vector<std::size_t> path{node};
        while(nodes_[path.back()].parent)
            path.push_back(nodes_[path.back()].parent.value());
        std::reverse(path.begin(), path.end());
        auto steps = nlohmann::json::array();
        for(auto index : path) {
            const auto& found = nodes_[index];
            nlohmann::json step{{"state", describer_.state(found.state)}};
            if(found.input)
                step["step"] = {{"label", describer_.input_label(found.input.value())}, {"kind", "input"},
                                {"elements", nlohmann::json::array()}};
            else if(found.parent)
                step["step"] = {{"label", describer_.label(found.edges)}, {"kind", "step"},
                                {"elements", describer_.elements_of(found.edges)}};
            steps.push_back(std::move(step));
        }
        return {{"steps", steps}};
    }

    auto search::result() const -> nlohmann::json {
        auto status = status_ == status_t::satisfied ? "satisfied" : status_ == status_t::violated ? "violated" : "unknown";
        nlohmann::json result{{"status", status}, {"message", message_},
                              {"statistics", {{"states", nodes_.size()}, {"searched", searched_}, {"transitions", transitions_}}}};
        if(decided_)
            result["trace"] = trace(decided_.value());
        return result;
    }
}
