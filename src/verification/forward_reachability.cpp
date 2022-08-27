#include "forward_reachability.h"
#include "ctl_sat.h"

namespace aaltitoad {
    // TODO: <feature request> todo: add a new issue on github
    //       Max search time? / max explosion allowed? - could be done with sizeof(s0) / GiB/MiB
    //       we could do a prompt: "reached RAM limit with xxx states remaining, continue?"
    //       With that, we could also do a Waiting-list velocity calculation. Maybe even output
    //       waiting-list size to a CSV file so it could be graphed! (very useful for papers)
    //       This behavior can be embedded into a specialization of traceable_multimap<T>
    auto forward_reachability_searcher::is_reachable(const aaltitoad::ntta_t& s0, const ctl::compiler::compiled_expr_t& q) -> bool {
        // TODO: statistics on all return statements (info)
        // TODO: Catch SIGTERM's (ctrl-c) and write statistics (info)
        // TODO: Periodically print waiting list size for debugging purposes (debug)
        W = {s0}; P = {};
        while(!W.empty()) {
            auto s = W.pop();
            auto s_it = P.add(s.parent, s.data);
            if(is_satisfied(q, s.data)) // TODO: List of queries
                return true; // TODO: return traceable results - return the newly added state

            for(auto& si : s.data.tick()) {
                auto sn = s.data + si;
                if(P.contains(sn))
                    continue;

                auto sn_it = P.add(s_it, sn);
                if(is_satisfied(q, sn)) // TODO: List of queries
                    return true; // TODO: return traceable results - return the newly added state

                for(auto& so : sn.tock())
                    W.add(sn_it, sn + so);
            }
        }
        return false; // TODO: return traceable results
    }
}

auto operator<<(std::ostream& o, const aaltitoad::forward_reachability_searcher::solution_t& s) -> std::ostream& {
    // this is stack-abuse... I'm calling the cops
    if(s->second.parent.has_value())
        o << s->second.parent.value();
    return o << s->second.data;
}
