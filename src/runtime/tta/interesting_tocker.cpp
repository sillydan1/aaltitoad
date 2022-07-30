#include "interesting_tocker.h"
#include <drivers/z3_driver.h>

namespace aaltitoad {
    auto interesting_tocker::thing(const ya::combiner_iterator_list_t<expr::syntax_tree_t> &elements) -> std::optional<expr::symbol_table_t> {
        expr::syntax_tree_t expression{};

        return {};
    }

    auto interesting_tocker::tock(const ntta_t& state) -> std::vector<expr::symbol_table_t> {
        expr::z3_driver d{state.symbols, state.external_symbols};
        // TODO: Go through all interesting edges from the current state
        std::vector<std::vector<expr::syntax_tree_t>> guards;
        for(auto& component : state.components) {
            // TODO: Cache the interesting edges for easy lookups later (can be done later) (lookup by component-location identifier)
            std::vector<expr::syntax_tree_t> interesting_guards{};
            for(auto& edge : component.second.current_location->second.outgoing_edges) {
                if(contains_external_variables(edge->second.data.guard)) {
                    interesting_guards.push_back(edge->second.data.guard);
                    interesting_guards.push_back(expr::syntax_tree_t{expr::operator_t{expr::operator_type_t::_not}}.concat(edge->second.data.guard));
                }
            }
            if(!interesting_guards.empty())
                guards.push_back(interesting_guards);
        }
        // TODO: Maybe implement a "only-add-changes-to-already-existing-variables"-apply function for symbol_table_t
        //       This is because known and unknown symbols are not interchangeable
        auto xx = [this, &d](const ya::combiner_iterator_list_t<expr::syntax_tree_t>& elements){
            return thing(d, elements);
        };
        return ya::generate_permutations(guards, xx);
    }
}
