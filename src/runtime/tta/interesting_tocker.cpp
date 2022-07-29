#include "interesting_tocker.h"
#include <drivers/z3_driver.h>

namespace aaltitoad {
    auto interesting_tocker::tock(const ntta_t& state) const -> std::vector<expr::symbol_table_t> {
        expr::z3_driver d{state.symbols, state.external_symbols};
        // TODO: Go through all interesting edges from the current state
        // TODO: Cache the interesting edges for easy lookups later (can be done later)
        // TODO: Find assignments of the guard "x" and "!(x)" (maybe threaded?)
        expr::syntax_tree_t tree{};
        auto negated = expr::syntax_tree_t{expr::operator_t{expr::operator_type_t::_not}}.concat(tree);
        d.result = {};
        d.add_tree(negated); // Catch exceptions here
        auto negative = d.result;

        // TODO: How do we combine the positives and negatives? - ya::permutation
        // TODO: How do we apply these changes? - in a simulator, just mash all the changes together - in verifier, every element is a new choice
        // TODO: Maybe implement a "only-add-changes-to-already-existing-variables"-apply function for symbol_table_t
        //       This is because known and unknown symbols are not interchangeable
        return {};
    }
}
