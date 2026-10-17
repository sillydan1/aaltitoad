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
#include "forward_reachability.h"
#include "spdlog/spdlog.h"
#include "verification/ctl/ctl_sat.h"
#include "verification/traceable_multimap.h"

namespace aaltitoad {
    namespace {
        // A G p is a safety query. Every other query is searched for like E F p
        auto kind_of(const ctl::syntax_tree_t& query) -> forward_reachability_searcher::query_kind_t {
            auto node = [](const ctl::syntax_tree_t& tree) -> const ctl::underlying_syntax_node_t& { return tree.node; };
            const auto* tree = &query;
            while(std::holds_alternative<expr::root_t>(node(*tree)) && tree->children().size() == 1)
                tree = &tree->children()[0];
            const auto* modal = std::get_if<ctl::modal_t>(&node(*tree));
            if(modal == nullptr || modal->operator_type != ctl::modal_op_t::A || tree->children().size() != 1)
                return forward_reachability_searcher::query_kind_t::reachability;
            const auto* quantifier = std::get_if<ctl::quantifier_t>(&node(tree->children()[0]));
            if(quantifier == nullptr || quantifier->operator_type != ctl::quantifier_op_t::G)
                return forward_reachability_searcher::query_kind_t::reachability;
            return forward_reachability_searcher::query_kind_t::safety;
        }
    }

    forward_reachability_searcher::query_solution_t::query_solution_t(compiled_query_t query)
     : query{std::move(query)}, kind{kind_of(this->query)}, solution{} {

    }

    auto forward_reachability_searcher::query_solution_t::is_decided() const -> bool {
        return solution.has_value() || is_exhaustive;
    }

    auto forward_reachability_searcher::query_solution_t::is_satisfied() const -> bool {
        if(!is_decided())
            return false;
        if(kind == query_kind_t::safety)
            return !solution.has_value();
        return solution.has_value();
    }

    auto forward_reachability_searcher::query_solution_t::is_solution(const ntta_t& state) const -> bool {
        auto holds = aaltitoad::is_satisfied(query, state);
        if(kind == query_kind_t::safety)
            return !holds;
        return holds;
    }

    forward_reachability_searcher::forward_reachability_searcher(const aaltitoad::pick_strategy& strategy, std::function<bool()> interrupted)
     : W{}, P{}, solutions{}, strategy{strategy}, interrupted{std::move(interrupted)} {

    }

    auto forward_reachability_searcher::is_reachable(const ntta_t& s0, const compiled_query_t& q) -> solutions_t {
        return is_reachable(s0, std::vector{q});
    }

    auto forward_reachability_searcher::is_reachable(const aaltitoad::ntta_t& s0, const std::vector<compiled_query_t>& q) -> solutions_t {
        W = {s0}; P = {}; solutions = empty_solution_set(q);
        auto s0_it = P.add(s0);
        for(auto& l : s0.tock()) {
            auto sp = s0 + l;
            if(!P.contains(sp))
                W.add_if_not_contains(s0_it, sp);
        }
        while(!W.empty()) {
            if(is_interrupted())
                return get_results();
            /// Select the next state to search
            auto s = W.pop(strategy);
            auto s_it = P.add(s.parent, s.data);
            if(check_satisfactions(s_it))
                return get_results();
            /// Add successors
            for(auto& si : s.data.tick()) {
                if(is_interrupted())
                    return get_results();
                auto sn = s.data + si;
                if(P.contains(sn))
                    continue;
                /// Calculate interesting tock changes
                auto sn_tocks = sn.tock();
                /// if nothing interesting is possible, just add tick-space state to W
                if(sn_tocks.empty()) {
                    W.add_if_not_contains(s_it, sn);
                    continue;
                }
                /// Add tock-space states to W
                spdlog::trace("{0} tock values available", sn_tocks.size());
                auto sn_it = P.add(s_it, sn);
                if(check_satisfactions(sn_it))
                    return get_results();
                for(auto& so : sn_tocks) {
                    auto sp = sn + so;
                    if(!P.contains(sp))
                        W.add_if_not_contains(sn_it, sp);
                }
            }
        }
        /// Searched through all of the reachable state-space from s0
        spdlog::debug("end of reachable state-space");
        for(auto& solution : solutions)
            solution.is_exhaustive = true;
        return get_results();
    }

    auto forward_reachability_searcher::is_interrupted() -> bool {
        if(!interrupted || !interrupted())
            return false;
        spdlog::warn("the search was interrupted after searching {0} states ({1} states were waiting)", P.size(), W.size());
        return true;
    }

    auto forward_reachability_searcher::empty_solution_set(const std::vector<compiled_query_t>& qs) -> solutions_t {
        solutions_t s{};
        for(auto& q : qs)
            s.emplace_back(q);
        return s;
    }

    auto forward_reachability_searcher::check_satisfactions(const solution_t& s) -> bool {
        for(auto& solution : solutions) {
            if(solution.solution.has_value()) continue;
            if(solution.is_solution(s->second.data))
                solution.solution = s;
        }
        return std::all_of(solutions.begin(), solutions.end(), [](const query_solution_t& sol){ return sol.solution.has_value(); });
    }

    auto forward_reachability_searcher::count_solutions() -> size_t {
        return std::accumulate(solutions.begin(), solutions.end(), 0, [&](size_t acc, const query_solution_t& a) {
            if(a.solution.has_value())
                return acc+1;
            return acc;
        });
    }

    auto forward_reachability_searcher::get_results() -> solutions_t {
        spdlog::info("[{0}/{1}] queries with solutions (len(P)={2})", count_solutions(), solutions.size(), P.size());
        return solutions;
    }
}

auto operator<<(std::ostream& o, const aaltitoad::forward_reachability_searcher::solution_t& s) -> std::ostream& { // NOLINT(misc-no-recursion)
    // this is stack-abuse... I'm calling the cops
    if(s->second.parent.has_value())
        o << s->second.parent.value();
    return o << s->second.data;
}

void to_json_helper(const aaltitoad::forward_reachability_searcher::solution_t& s, std::vector<nlohmann::json>& tail) {
    if(s->second.parent.has_value())
        to_json_helper(s->second.parent.value(), tail);
    tail.push_back(s->second.data.to_json());
}

auto to_json(const aaltitoad::forward_reachability_searcher::solution_t& s) -> std::vector<nlohmann::json> {
    std::vector<nlohmann::json> states{};
    to_json_helper(s, states);
    return states;
}

