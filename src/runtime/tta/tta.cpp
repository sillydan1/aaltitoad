#include "tta.h"

namespace aaltitoad {
    auto ntta_t::tick() const -> state_change_t {
        return {};
    }
    void ntta_t::tick(const state_change_t &changes)  {
        for(auto& location_change : changes.location_changes)
            location_change.component->second.current_location = location_change.new_location;
        symbols += changes.symbol_changes;
    }

    auto ntta_t::tock() const -> expr::symbol_table_t {
        return {};
    }
    void ntta_t::tock(const expr::symbol_table_t &symbol_changes) {
        symbols += symbol_changes;
    }
}
