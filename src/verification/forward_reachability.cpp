#include "forward_reachability.h"
#include "ctl_sat.h"

namespace aaltitoad {
    auto forward_reachability_searcher::is_reachable(const aaltitoad::ntta_t& s0, const ctl::compiler::compiled_expr_t& q) -> bool {
        // TODO: List of queries
        // TODO: statistics on all return statements (info)
        // TODO: Catch SIGTERM's and write statistics (info)
        // TODO: Periodically print waiting list size for debugging purposes (debug)
        // TODO: Max search time? / max explosion allowed? nah
        W = {s0}; P = {};
        while(!W.empty()) {
            auto s = W.pop();
            if(is_satisfied(q, s))
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
