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
#include "forward_reachability.h"
#include "verification/ctl/ctl_sat.h"

namespace aaltitoad {
    forward_reachability_searcher::forward_reachability_searcher(const aaltitoad::pick_strategy& strategy)
     : W{}, P{}, solutions{}, strategy{strategy} {

    }

    auto forward_reachability_searcher::is_reachable(const ntta_t& s0, const compiled_query_t& q) -> solutions_t {
        return is_reachable(s0, std::vector{q});
    }

    auto forward_reachability_searcher::is_reachable(const aaltitoad::ntta_t& s0, const std::vector<compiled_query_t>& q) -> solutions_t {
        // TODO: Catch SIGTERM (ctrl-c) and write statistics (info)
        W = {s0}; P = {}; solutions = empty_solution_set(q);
        auto s0_it = P.add(s0);
        for(auto& l : s0.tock()) {
            auto sp = s0 + l;
            if(!P.contains(sp))
                W.add(s0_it, sp);
        }
        while(!W.empty()) {
            spdlog::trace("len(W)={0} | len(P)={1}", W.size(), P.size());
            /// Select the next state to search
            auto s = W.pop(strategy);
            /// Does this state complete our query-list?
            auto s_it = P.add(s.parent, s.data);
            if(check_satisfactions(s_it))
                return get_results();
            /// Add successors
            for(auto& si : s.data.tick()) {
                auto sn = s.data + si;
                if(P.contains(sn))
                    continue;
                /// Calculate interesting tock changes
                auto sn_tocks = sn.tock();
                /// if nothing interesting is possible, just add tick-space state to W
                if(sn_tocks.empty()) {
                    W.add(s_it, sn);
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
                        W.add(sn_it, sp);
                }
            }
        }
        /// Searched through all of the reachable state-space from s0
        spdlog::info("end of reachable state-space");
        return get_results();
    }

    auto forward_reachability_searcher::empty_solution_set(const std::vector<compiled_query_t>& qs) -> solutions_t {
        solutions_t s{};
        for(auto& q : qs)
            s.push_back({q});
        return s;
    }

    auto forward_reachability_searcher::check_satisfactions(const solution_t& s) -> bool {
        // TODO: With AG queries, they are always "true" until you find a counter-example, then they are "false", but with a solution
        //       right now, we are doing the opposite
        for(auto& solution : solutions) {
            if(solution.solution.has_value()) continue;
            if(is_satisfied(solution.query, s->second.data))
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
