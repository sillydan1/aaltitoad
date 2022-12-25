/**
 * aaltitoad - a verification engine for tick tock automata models
   Copyright (C) 2023 Asger Gitz-Johansen

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include "interesting_tocker.h"
#include <drivers/z3_driver.h>
#include <spdlog/spdlog.h>

namespace aaltitoad {
    auto interesting_tocker::find_solution(expr::z3_driver& d, const ya::combiner_iterator_list_t<expr::syntax_tree_t> &elements) -> std::optional<expr::symbol_table_t> {
        if(elements.empty())
            return {};
        auto it = elements.begin();
        auto expression = **it;
        if(elements.size() > 1)
            for(it++; it != elements.end(); it++)
                expression = expr::syntax_tree_t{expr::operator_t{expr::operator_type_t::_and}}
                        .concat(expression)
                        .concat(**it);
        try {
            d.result = {};
            d.add_tree(expression);
            if (!d.result.empty() || d.result.get_delay_amount().has_value())
                return std::optional{d.result};
        } catch(std::domain_error& e) {
            std::stringstream ss{}; ss << expression;
            spdlog::trace("'{0}' for '{1}'", e.what(), ss.str());
        } catch(std::exception& e) {
            spdlog::error("error during tock step evaluation: '{0}'", e.what());
        }
        return {};
    }

    auto interesting_tocker::tock(const ntta_t& state) -> std::vector<expr::symbol_table_t> {
        std::vector<std::vector<expr::syntax_tree_t>> guards;
        for(auto& component : state.components) {
            // If this is slow, we should investigate maintaining a cache to avoid iteration
            std::vector<expr::syntax_tree_t> interesting_guards{};
            for(auto& edge : component.second.current_location->second.outgoing_edges) {
                auto is_interesting = contains_external_variables(edge->second.data.guard, state.external_symbols) ||
                                      contains_timer_variables(edge->second.data.guard, state.symbols);
                if(is_interesting) {
                    interesting_guards.push_back(edge->second.data.guard);
                    interesting_guards.push_back(expr::syntax_tree_t{expr::operator_t{expr::operator_type_t::_not}}.concat(edge->second.data.guard));
                }
            }
            if(!interesting_guards.empty())
                guards.push_back(interesting_guards);
        }
        if(guards.empty())
            return {};
        expr::z3_driver d{state.symbols, state.external_symbols};
        ya::combiner_funct_t<expr::symbol_table_t, expr::syntax_tree_t> f =
                [&d](const ya::combiner_iterator_list_t<expr::syntax_tree_t>& elements) -> std::optional<expr::symbol_table_t> {
            return find_solution(d, elements);
        };
        auto perms = ya::generate_permutations(guards, f);
        spdlog::debug("{0} interesting guards generated {1} permutations", guards.size(), perms.size());
        return perms;
    }

    auto interesting_tocker::contains_timer_variables(const expr::syntax_tree_t& tree, const expr::symbol_table_t& symbols) const -> bool {
        auto expr_clock_index = expr::symbol_value_t{expr::clock_t{0}}.index();
        return std::visit(ya::overload(
                [&symbols, &expr_clock_index](const expr::identifier_t& r){
                    if(symbols.contains(r.ident))
                        if(symbols.find(r.ident)->second.index() == expr_clock_index)
                            return true;
                    return false;
                },
                [this, &tree, &symbols](const expr::root_t& r){
                    if(tree.children().empty())
                        return false;
                    return contains_timer_variables(tree.children()[0], symbols);
                },
                [this, &tree, &symbols](const expr::operator_t& r){
                    return std::any_of(tree.children().begin(), tree.children().end(),
                                       [&](const auto& c){
                                           return contains_timer_variables(c, symbols);
                                       });
                },
                [](auto&&){ return false; }
        ), static_cast<const expr::underlying_syntax_node_t&>(tree.node));
    }

    auto interesting_tocker::contains_external_variables(const expr::syntax_tree_t& tree, const expr::symbol_table_t& symbols) const -> bool {
        return std::visit(ya::overload(
                [&symbols](const expr::identifier_t& r){ return symbols.contains(r.ident); },
                [this, &tree, &symbols](const expr::root_t& r){
                    if(tree.children().empty())
                        return false;
                    return contains_external_variables(tree.children()[0], symbols);
                },
                [this, &tree, &symbols](const expr::operator_t& r){
                    return std::any_of(tree.children().begin(), tree.children().end(),
                                       [&](const auto& c){
                                           return contains_external_variables(c, symbols);
                                       });
                },
                [](auto&&){ return false; }
        ), static_cast<const expr::underlying_syntax_node_t&>(tree.node));
    }

    auto interesting_tocker::get_name() -> std::string {
        return "interesting_tocker";
    }
}
