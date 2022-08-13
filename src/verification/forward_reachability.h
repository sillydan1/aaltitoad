#ifndef AALTITOAD_FORWARD_REACHABILITY_H
#define AALTITOAD_FORWARD_REACHABILITY_H
#include "ctl.h"
#include "ntta/tta.h"

namespace aaltitoad {
    class forward_reachability_searcher {
    public:
        auto is_reachable(const ntta_t& model, const ctl::query& q) -> bool;
    };
}

#endif //AALTITOAD_FORWARD_REACHABILITY_H
