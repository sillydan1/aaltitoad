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
#include "interpreter.h"
#include "symbol_table.h"

auto operator<<(std::ostream& os, const expr::syntax_tree_collection_t& c) -> std::ostream& {
    for(auto& e : c)
        os << e.first << " :-> " << e.second << ", ";
    return os;
}

namespace aaltitoad {
    auto expression_driver::language_result::get_symbol_table() -> expr::symbol_table_t {
        expr::symbol_operator op{};
        expr::evaluator e{{}, op};
        expr::symbol_table_t env{};
        for(auto& r : declarations)
            env[r.first] = e.evaluate(r.second);
        return env;
    }

    auto expression_driver::language_result::get_symbol_value() -> expr::symbol_value_t {
        expr::symbol_operator op{};
        expr::evaluator e{{}, op};
        return e.evaluate(expression.value());
    }

    expression_driver::expression_driver() : known_environment{}, unknown_environment{} {}
    expression_driver::expression_driver(const expr::symbol_table_t& env) : known_environment{env}, unknown_environment{} {}
    expression_driver::expression_driver(const expr::symbol_table_t& env0, const expr::symbol_table_t& env1) : known_environment{env0}, unknown_environment{env1} {}
    expression_driver::~expression_driver() {}
    auto expression_driver::evaluate(const expr::syntax_tree_collection_t& declarations) -> expr::symbol_table_t {
        expr::evaluator e{{known_environment, unknown_environment}, expr::symbol_operator{}};
        expr::symbol_table_t result{};
        for(auto& decl : declarations)
            result[decl.first] = e.evaluate(decl.second);
        return result;
    }

    auto expression_driver::evaluate(const expr::syntax_tree_t& expression) -> expr::symbol_value_t {
        return expr::evaluator{{known_environment, unknown_environment}, expr::symbol_operator{}}.evaluate(expression);
    }

    auto expression_driver::sat_check(const expr::syntax_tree_t& expression) -> expr::symbol_table_t {
        auto solultion = expr::z3_driver{known_environment, unknown_environment}.find_solution(expression);
        if(solultion)
            return solultion.value();
        return {};
    }

    auto expression_driver::parse_guard(const std::string& expression) -> expr::syntax_tree_t {
        if(expression.empty()) {
            expr::ast_factory factory{};
            return factory.build_root(factory.build_literal(true)) ;
        }
        return parse(expression).expression.value(); 
    }

    auto expression_driver::parse(const std::string& s) -> language_result {
        std::istringstream iss{s};
        expr::ast_factory factory{};
        expr::declaration_tree_builder builder{};
        expr::scanner sc{iss, std::cerr, &factory};
        expr::parser_args pa{s, &sc, &factory, &builder};
        expr::parser p{pa};
        if(p.parse() != 0)
            throw std::logic_error("expression_driver: unable to parse the expression(s): " + s);
        auto res = builder.build();
        language_result result{};
        for(auto& e : res.declarations)
            result.declarations[e.first] = e.second.tree;
        if(res.raw_expression)
            result.expression = res.raw_expression.value();
        return result;
    }
}

