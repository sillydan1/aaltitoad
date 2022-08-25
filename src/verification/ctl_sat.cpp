#include "ctl_sat.h"

namespace aaltitoad {
    auto eval(const expr::syntax_tree_t& t, expr::interpreter& i) {
        return expr::interpreter::evaluate(t,i,i,i);
    }

    auto is_satisfied(const ctl::compiler::compiled_expr_t& ast, const ntta_t& state) -> bool {
        // TODO: This does not work if the ast is more complex than E F predicate
        auto value = false;
        std::visit(ya::overload(
                [&](const expr::syntax_tree_t& v)   {
                    expr::interpreter i{state.symbols + state.external_symbols};
                    value = std::get<bool>(eval(v, i));
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
                        case ctl::modal_op_t::A: value = is_satisfied(ast.children[0], state); break;
                        case ctl::modal_op_t::E: value = is_satisfied(ast.children[0], state); break;
                    }},
                [&](const ctl::quantifier_t &v)     {
                    switch(v) {
                        case ctl::quantifier_t::X: value = is_satisfied(ast.children[0], state); break;
                        case ctl::quantifier_t::F: value = is_satisfied(ast.children[0], state); break;
                        case ctl::quantifier_t::G: value = is_satisfied(ast.children[0], state); break;
                        // TODO: actual modal/quantifier interpretation (U & W have 2 children btw)
                        case ctl::quantifier_t::U: value = is_satisfied(ast.children[0], state); break;
                        case ctl::quantifier_t::W: value = is_satisfied(ast.children[0], state); break;
                    }},
                [&](const expr::operator_t& v)      {
                    switch(v.operator_type) {
                        case expr::operator_type_t::_and:
                            value = is_satisfied(ast.children[0], state) && is_satisfied(ast.children[1], state); break;
                        case expr::operator_type_t::_or:
                            value = is_satisfied(ast.children[0], state) || is_satisfied(ast.children[1], state); break;
                        case expr::operator_type_t::_xor:
                            value = is_satisfied(ast.children[0], state) != is_satisfied(ast.children[1], state); break;
                        case expr::operator_type_t::_implies:
                            value = !is_satisfied(ast.children[0], state) || is_satisfied(ast.children[1], state); break;
                        case expr::operator_type_t::_not:
                            value = !is_satisfied(ast.children[0], state); break;
                        default: throw std::logic_error("not a valid raw CTL operator");
                    }}
        ), ast.node);
        return value;
    }
}
