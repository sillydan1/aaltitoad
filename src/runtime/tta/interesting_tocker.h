#ifndef AALTITOAD_INTERESTING_TOCKER_H
#define AALTITOAD_INTERESTING_TOCKER_H
#include "tta.h"
#include <drivers/z3_driver.h>

namespace aaltitoad {
    class interesting_tocker : public tocker_t {
    public:
        [[nodiscard]] auto tock(const ntta_t& state) const -> std::vector<expr::symbol_table_t> override;
        ~interesting_tocker() override = default;
    private:
        auto find_solution(expr::z3_driver& driver, const expr::syntax_tree_t& guard) -> expr::symbol_table_t;
    };
}

#endif //AALTITOAD_INTERESTING_TOCKER_H
