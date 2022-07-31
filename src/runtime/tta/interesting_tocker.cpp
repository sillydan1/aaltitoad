#include "interesting_tocker.h"
#include <drivers/z3_driver.h>
#include <spdlog/spdlog.h>

namespace aaltitoad {
    auto interesting_tocker::find_solution(expr::z3_driver& d, const ya::combiner_iterator_list_t<expr::syntax_tree_t> &elements) -> std::optional<expr::symbol_table_t> {
        if(elements.empty())
            return {};
        auto it = elements.begin();
        auto expression = **it;
        if(elements.size() > 1) {
            it++;
            for(; it != elements.end(); it++) {
                expression = expr::syntax_tree_t{expr::operator_t{expr::operator_type_t::_and}}
                        .concat(expression)
                        .concat(**it);
            }
        }
        try {
            d.result = {};
            d.add_tree(expression);
            if (d.result.empty())
                return {};
            return std::optional{d.result};
        } catch(std::domain_error& e) {
            spdlog::trace("encountered unsatisfiable tock step ignoring");
        } catch(std::exception& e) {
            spdlog::warn("error during tock step evaluation: '{0}'", e.what());
        }
        return {};
    }

    auto interesting_tocker::tock(const ntta_t& state) -> std::vector<expr::symbol_table_t> {
        expr::z3_driver d{state.symbols, state.external_symbols};
        std::vector<std::vector<expr::syntax_tree_t>> guards;
        for(auto& component : state.components) {
            // TODO: Cache the interesting edges for easy lookups later (can be done later) (lookup by component-location identifier)
            std::vector<expr::syntax_tree_t> interesting_guards{};
            for(auto& edge : component.second.current_location->second.outgoing_edges) {
                if(contains_external_variables(edge->second.data.guard, state.external_symbols)) {
                    interesting_guards.push_back(edge->second.data.guard);
                    interesting_guards.push_back(expr::syntax_tree_t{expr::operator_t{expr::operator_type_t::_not}}.concat(edge->second.data.guard));
                }
            }
            if(!interesting_guards.empty())
                guards.push_back(interesting_guards);
        }
        // TODO: Maybe implement a "only-add-changes-to-already-existing-variables"-apply function for symbol_table_t
        //       This is because known and unknown symbols are not interchangeable
        ya::combiner_funct_t<expr::symbol_table_t, expr::syntax_tree_t> f = [&d](const ya::combiner_iterator_list_t<expr::syntax_tree_t>& elements) -> std::optional<expr::symbol_table_t> {return find_solution(d, elements);};
        return ya::generate_permutations(guards, f);
    }

    auto interesting_tocker::contains_external_variables(const expr::syntax_tree_t& tree, const expr::symbol_table_t& symbols) const -> bool {
        bool return_value = false;
        std::visit(ya::overload(
                [&symbols, &return_value](const expr::symbol_reference_t& r){ return_value |= symbols.contains(r->first); },
                [&symbols, &return_value](const expr::c_symbol_reference_t& r){ return_value |= symbols.contains(r->first); },
                [this, &tree, &symbols, &return_value](const expr::root_t& r){ return_value |= contains_external_variables(tree.children[0], symbols); },
                [this, &tree, &symbols, &return_value](const expr::operator_t& r){
                    for(auto& c : tree.children) {
                        return_value |= contains_external_variables(c, symbols);
                        if(return_value)
                            return;
                    }
                },
                [](auto&&){}
        ), tree.node);
        return return_value;
    }
}