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
#include "symbol_table.h"
#include <ctl_syntax_tree.h>
#include <variant>

namespace aaltitoad {
    auto is_satisfied(const ctl::syntax_tree_t& ast, const ntta_t& state) -> bool {
        // TODO: This does not work if the ast is more complex than E F predicate (https://github.com/sillydan1/aaltitoad/issues/41)
        return std::visit(ya::overload(
                              [&](const expr::syntax_tree_t& v) -> bool {
                                  auto s = state.symbols + state.external_symbols;
                                  return std::get<bool>(expression_driver{s}.evaluate(v));
                              },
                              [&](const expr::root_t& v) -> bool {
                                  return is_satisfied(ast.children()[0], state);
                              },
                              [&](const ctl::location_t &v) -> bool {
                                  for(auto& component : state.components)
                                      if(component.second.current_location->first == v.location_name)
                                          return true;
                                  return false;
                              },
                              [&](const ctl::modal_t &v) -> bool {
                                  switch(v.operator_type) {
                                      case ctl::modal_op_t::A: return is_satisfied(ast.children()[0], state);
                                      case ctl::modal_op_t::E: return is_satisfied(ast.children()[0], state);
                                      default: throw std::logic_error("not a valid CTL modal");
                                  }},
                              [&](const ctl::quantifier_t &v) -> bool {
                                  switch(v.operator_type) {
                                      case ctl::quantifier_op_t::X: return is_satisfied(ast.children()[0], state);
                                      case ctl::quantifier_op_t::F: return is_satisfied(ast.children()[0], state);
                                      case ctl::quantifier_op_t::G: return is_satisfied(ast.children()[0], state);
                                      case ctl::quantifier_op_t::U: return is_satisfied(ast.children()[0], state);
                                      case ctl::quantifier_op_t::W: return is_satisfied(ast.children()[0], state);
                                      default: throw std::logic_error("not a valid CTL quantifier");
                                  }},
                              [&](const expr::operator_t& v) -> bool {
                                  switch(v.operator_type) {
                                      case expr::operator_type_t::_and:     return is_satisfied(ast.children()[0], state) && is_satisfied(ast.children()[1], state);
                                      case expr::operator_type_t::_or:      return is_satisfied(ast.children()[0], state) || is_satisfied(ast.children()[1], state);
                                      case expr::operator_type_t::_xor:     return is_satisfied(ast.children()[0], state) != is_satisfied(ast.children()[1], state);
                                      case expr::operator_type_t::_implies: return !is_satisfied(ast.children()[0], state) || is_satisfied(ast.children()[1], state);
                                      case expr::operator_type_t::_not:     return !is_satisfied(ast.children()[0], state);
                                      default: throw std::logic_error("not a valid CTL operator");
                                  }},
                              [](auto&&) -> bool { throw std::logic_error("unsupported CTL syntax_tree node type"); }
                          ), static_cast<const ctl::underlying_syntax_node_t&>(ast.node));
    }
}

