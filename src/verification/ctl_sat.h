#ifndef AALTITOAD_CTL_SAT_H
#define AALTITOAD_CTL_SAT_H
#include <ctl_compiler.h>
#include <ntta/tta.h>

namespace aaltitoad {
    auto is_satisfied(const ctl::compiler::compiled_expr_t& ast, const ntta_t& state) -> bool;
}

#endif //AALTITOAD_CTL_SAT_H
