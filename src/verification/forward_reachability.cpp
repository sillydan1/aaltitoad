#include "forward_reachability.h"

namespace aaltitoad {
    auto forward_reachability_searcher::is_reachable(const aaltitoad::ntta_t& s0, const ctl::query &q) -> bool {
        W = {s0}; P = {};
        while(!W.empty()) {
            auto s = W.pop();
            if(ctl::is_satisfied(q, s))
                return true; // TODO: traceability
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
        // TODO: statistics on all return statements
        return false; // TODO: traceability
    }
}
