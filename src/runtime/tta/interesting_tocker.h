#ifndef AALTITOAD_INTERESTING_TOCKER_H
#define AALTITOAD_INTERESTING_TOCKER_H
#include "tta.h"

namespace aaltitoad {
    class interesting_tocker : public tocker_t {
        [[nodiscard]] auto tock(const ntta_t& state) const -> std::vector<expr::symbol_table_t> override;
        ~interesting_tocker() override = default;
    };
}

#endif //AALTITOAD_INTERESTING_TOCKER_H
