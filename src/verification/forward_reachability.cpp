#include "forward_reachability.h"
#include "ctl_sat.h"

namespace aaltitoad {
    // TODO: <feature request> todo: add a new issue on github
    //       Max search time? / max explosion allowed? - could be done with sizeof(s0) / GiB/MiB
    //       we could do a prompt: "reached RAM limit with xxx states remaining, continue?"
    //       With that, we could also do a Waiting-list velocity calculation. Maybe even output
    //       waiting-list size to a CSV file so it could be graphed! (very useful for papers)
    //       This behavior can be embedded into a specialization of colliding_multimap<T>
    auto forward_reachability_searcher::is_reachable(const aaltitoad::ntta_t& s0, const ctl::compiler::compiled_expr_t& q) -> bool {
        // TODO: statistics on all return statements (info)
        // TODO: Catch SIGTERM's (ctrl-c) and write statistics (info)
        // TODO: Periodically print waiting list size for debugging purposes (debug)
        W = {s0}; P = {};
        while(!W.empty()) {
            auto s = W.pop();
            if(is_satisfied(q, s)) // TODO: List of queries
                return true; // TODO: return traceable results
            for(auto& si : s.tick()) {
                auto successor = s + si;
                if(P.contains(successor))
                    continue;
                W.add(successor);
                for(auto& so : successor.tock())
                    W.add(successor + so);
            }
            P.add(s);
        }
        return false; // TODO: return traceable results
    }
}
