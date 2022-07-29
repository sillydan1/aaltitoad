#include "interesting_tocker.h"
#include <drivers/z3_driver.h>

namespace aaltitoad {
    auto interesting_tocker::tock(const ntta_t& state) const -> std::vector<expr::symbol_table_t> {
        expr::z3_driver d{state.symbols, state.external_symbols};
        // TODO: Go through all interesting edges from the current state
        // TODO: Cache the interesting edges for easy lookups later
        // TODO: How do we combine the positives and negatives?
        // TODO: What do we do if there are conflicting variable assignments?
        // TODO: How do we apply these changes?
        // TODO: Maybe implement a "only-add-changes-to-already-existing-variables"-apply function for symbol_table_t
        return {};
    }
}
