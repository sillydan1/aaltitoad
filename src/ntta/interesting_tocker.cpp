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
            std::stringstream ss{}; ss << expression;
            spdlog::trace("'{0}' for '{1}'", e.what(), ss.str());
        } catch(std::exception& e) {
            spdlog::warn("error during tock step evaluation: '{0}'", e.what());
        }
        return {};
    }

    auto interesting_tocker::tock(const ntta_t& state) -> std::vector<expr::symbol_table_t> {
        expr::z3_driver d{state.symbols, state.external_symbols};
        std::vector<std::vector<expr::syntax_tree_t>> guards;
        for(auto& component : state.components) {
            // If this is slow, we should investigate maintaining a cache to avoid iteration
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
        if(guards.empty())
            return {{}};
        ya::combiner_funct_t<expr::symbol_table_t, expr::syntax_tree_t> f =
                [&d](const ya::combiner_iterator_list_t<expr::syntax_tree_t>& elements) -> std::optional<expr::symbol_table_t> {
            return find_solution(d, elements);
        };
        auto perms = ya::generate_permutations(guards, f);
        spdlog::trace("{0} interesting guards generated {1} permutations", guards.size(), perms.size());
        return perms;
    }

    auto interesting_tocker::contains_external_variables(const expr::syntax_tree_t& tree, const expr::symbol_table_t& symbols) const -> bool {
        bool return_value = false;
        std::visit(ya::overload(
                [&symbols, &return_value](const expr::symbol_reference_t& r){ return_value |= symbols.contains(r->first); },
                [&symbols, &return_value](const expr::c_symbol_reference_t& r){ return_value |= symbols.contains(r->first); },
                [this, &tree, &symbols, &return_value](const expr::root_t& r){
                    if(!tree.children.empty())
                        return_value |= contains_external_variables(tree.children[0], symbols);
                },
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

    auto interesting_tocker::get_name() -> std::string {
        return "interesting_tocker";
    }
}
