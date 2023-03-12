#ifndef AALTITOAD_EXPR_WRAPPER_INTERPRETER_H
#define AALTITOAD_EXPR_WRAPPER_INTERPRETER_H
#include <ctl_syntax_tree.h>
#include <stdexcept>
#include <symbol_table.h>
#include <driver/z3/z3-driver.h>
#include "expr-lang/ast-factory.h"
#include "expr-lang/language-builder.h"
#include "expr-lang/expr-scanner.hpp"
#include "operations/symbol-operator.h"
#include <driver/evaluator.h>
#include <expr-parser.hpp>

namespace expr {
    using syntax_tree_collection_t = std::map<std::string, expr::syntax_tree_t>;
}

namespace aaltitoad {
    class expression_driver {
    public:
        struct language_result {
            expr::syntax_tree_collection_t declarations;
            std::optional<expr::syntax_tree_t> expression;
            auto get_symbol_table() -> expr::symbol_table_t { // NOTE: no environment context available here
                expr::symbol_operator op{};
                expr::evaluator e{{}, op};
                expr::symbol_table_t env{};
                for(auto& r : declarations)
                    env[r.first] = e.evaluate(r.second);
                return env;
            }
            auto get_symbol_value() -> expr::symbol_value_t {
                expr::symbol_operator op{};
                expr::evaluator e{{}, op};
                return e.evaluate(expression.value());
            }
        };
        expression_driver() : known_environment{}, unknown_environment{} {} // TODO: stop copying the environments, they should be reference_wrapper<const symbol_table_t> instead
        expression_driver(const expr::symbol_table_t& known) : known_environment{known}, unknown_environment{} {}
        expression_driver(const expr::symbol_table_t& known, const expr::symbol_table_t& unknown) : known_environment{known}, unknown_environment{unknown} {}
        virtual ~expression_driver() = default;

        auto evaluate(const expr::syntax_tree_collection_t& declarations) -> expr::symbol_table_t { // NOTE: this uses environment context
            expr::evaluator e{{known_environment, unknown_environment}, expr::symbol_operator{}};
            expr::symbol_table_t result{};
            for(auto& decl : declarations)
                result[decl.first] = e.evaluate(decl.second);
            return result;
        }

        auto evaluate(const expr::syntax_tree_t& expression) -> expr::symbol_value_t {
            return expr::evaluator{{known_environment, unknown_environment}, expr::symbol_operator{}}.evaluate(expression);
        }

        auto sat_check(const expr::syntax_tree_t& expression) -> expr::symbol_table_t {
            auto solultion = expr::z3_driver{known_environment, unknown_environment}.find_solution(expression);
            if(solultion)
                return solultion.value();
            return {};
        }

        auto parse_guard(const std::string& expression) -> expr::syntax_tree_t {
            if(expression.empty()) {
                expr::ast_factory factory{};
                return factory.build_root(factory.build_literal(true)) ;
            }
            return parse(expression).expression.value(); 
        }

        virtual auto parse(const std::string& s) -> language_result {
            std::istringstream iss{s};
            expr::ast_factory factory{};
            expr::declaration_tree_builder builder{};
            expr::scanner sc{iss, std::cerr, &factory};
            expr::parser_args pa{&sc, &factory, &builder};
            expr::parser p{pa};
            if(p.parse() != 0)
                throw std::logic_error("unable to parse the expression(s)");
            auto res = builder.build();
            language_result result{};
            for(auto& e : res.declarations)
                result.declarations[e.first] = e.second.tree;
            if(res.raw_expression)
                result.expression = res.raw_expression.value();
            return result;
        }

    public:
        expr::symbol_table_t known_environment{};
        expr::symbol_table_t unknown_environment{};
    };
}

#endif

