#include "forward_reachability.h"
#include "ctl_sat.h"

namespace aaltitoad {
    // TODO: <feature request> todo: add a new issue on github
    //       Max search time? / max explosion allowed? - could be done with sizeof(s0) / GiB/MiB
    //       we could do a prompt: "reached RAM limit with xxx states remaining, continue?"
    //       With that, we could also do a Waiting-list velocity calculation. Maybe even output
    //       waiting-list size to a CSV file so it could be graphed! (very useful for papers)
    //       This behavior can be embedded into a specialization of traceable_multimap<T>
    auto forward_reachability_searcher::is_reachable(const ntta_t& s0, const compiled_query_t& q) -> solutions_t {
        return is_reachable(s0, std::vector{q});
    }

    auto forward_reachability_searcher::is_reachable(const aaltitoad::ntta_t& s0, const std::vector<compiled_query_t>& q) -> solutions_t {
        // TODO: statistics on all return statements (info)
        // TODO: Catch SIGTERM (ctrl-c) and write statistics (info)
        // TODO: Periodically print waiting list size for debugging purposes (debug)
        W = {s0}; P = {}; solutions = empty_solution_set(q);
        while(!W.empty()) {
            auto s = W.pop();
            auto s_it = P.add(s.parent, s.data);
            if(check_satisfactions(s_it))
                return solutions;

            for(auto& si : s.data.tick()) {
                auto sn = s.data + si;
                if(P.contains(sn))
                    continue;

                auto sn_it = P.add(s_it, sn);
                if(check_satisfactions(sn_it))
                    return solutions;

                for(auto& so : sn.tock())
                    W.add(sn_it, sn + so);
            }
        }
        spdlog::trace("[len(P)={0}] end of reachable state-space", P.size());
        spdlog::trace("[{0}/{1}] queries with solutions", count_solutions(), solutions.size());
        return solutions;
    }

    auto forward_reachability_searcher::empty_solution_set(const std::vector<compiled_query_t>& qs) -> solutions_t {
        solutions_t s{};
        for(auto& q : qs)
            s.push_back({q});
        return s;
    }

    auto forward_reachability_searcher::check_satisfactions(const solution_t& s) -> bool {
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
}

auto operator<<(std::ostream& o, const aaltitoad::forward_reachability_searcher::solution_t& s) -> std::ostream& { // NOLINT(misc-no-recursion)
    // this is stack-abuse... I'm calling the cops
    if(s->second.parent.has_value())
        o << s->second.parent.value();
    return o << s->second.data;
}
