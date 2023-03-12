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
#include "ctl_sat.h"
#include "expr-wrappers/interpreter.h"
#include <ctl_syntax_tree.h>

namespace aaltitoad {
    auto is_satisfied(const ctl::syntax_tree_t& ast, const ntta_t& state) -> bool {
        // TODO: This does not work if the ast is more complex than E F predicate
        auto value = false;
        std::visit(ya::overload(
                [&](const expr::syntax_tree_t& v)   {
                    auto s = state.symbols + state.external_symbols;
                    value = std::get<bool>(expression_driver{s}.evaluate(v));
                    },
                [&](const ctl::location_t &v)       {
                    for(auto& component : state.components) {
                        if(component.second.current_location->first == v.location_name) {
                            value = true;
                            break;
                        }
                    }},
                [&](const ctl::modal_op_t &v)       {
                    switch(v) {
                        // TODO: actual modal/quantifier interpretation
                        case ctl::modal_op_t::A: value = is_satisfied(ast.children()[0], state); break;
                        case ctl::modal_op_t::E: value = is_satisfied(ast.children()[0], state); break;
                    }},
                [&](const ctl::quantifier_op_t &v)     {
                    switch(v) {
                        case ctl::quantifier_op_t::X: value = is_satisfied(ast.children()[0], state); break;
                        case ctl::quantifier_op_t::F: value = is_satisfied(ast.children()[0], state); break;
                        case ctl::quantifier_op_t::G: value = is_satisfied(ast.children()[0], state); break;
                        // TODO: actual modal/quantifier interpretation (U & W have 2 children btw)
                        case ctl::quantifier_op_t::U: value = is_satisfied(ast.children()[0], state); break;
                        case ctl::quantifier_op_t::W: value = is_satisfied(ast.children()[0], state); break;
                    }},
                [&](const expr::operator_t& v)      {
                    switch(v.operator_type) {
                        case expr::operator_type_t::_and:
                            value = is_satisfied(ast.children()[0], state) && is_satisfied(ast.children()[1], state); break;
                        case expr::operator_type_t::_or:
                            value = is_satisfied(ast.children()[0], state) || is_satisfied(ast.children()[1], state); break;
                        case expr::operator_type_t::_xor:
                            value = is_satisfied(ast.children()[0], state) != is_satisfied(ast.children()[1], state); break;
                        case expr::operator_type_t::_implies:
                            value = !is_satisfied(ast.children()[0], state) || is_satisfied(ast.children()[1], state); break;
                        case expr::operator_type_t::_not:
                            value = !is_satisfied(ast.children()[0], state); break;
                        default: throw std::logic_error("not a valid raw CTL operator");
                    }},
                [](auto&& v) {
                           std::cerr << "something" << std::endl; // TODO: Decide what to do here. If it is fine, dont print anything
                       }
        ), static_cast<const ctl::underlying_syntax_node_t&>(ast.node));
        return value;
    }
}

