#ifndef AALTITOAD_FORWARD_REACHABILITY_H
#define AALTITOAD_FORWARD_REACHABILITY_H
#include "ctl_sat.h"
#include "ntta/tta.h"
#include "traceable_multimap.h"
#include <ctl_compiler.h>

namespace aaltitoad {
    class forward_reachability_searcher {
        traceable_multimap<ntta_t> W, P;
    public:
        using solution_t = traceable_multimap<ntta_t>::iterator_t;
        auto is_reachable(const ntta_t& model, const ctl::compiler::compiled_expr_t& q) -> bool;
    };
}
auto operator<<(std::ostream& o, const aaltitoad::forward_reachability_searcher::solution_t& s) -> std::ostream&;

#endif //AALTITOAD_FORWARD_REACHABILITY_H
