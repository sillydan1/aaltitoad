#ifndef AALTITOAD_EXPR_WRAPPER_INTERPRETER_H
#define AALTITOAD_EXPR_WRAPPER_INTERPRETER_H
#include <symbol_table.h>
#include <driver/z3/z3-driver.h>
#include "../expr-src/src/lang/ast-factory.h"
#include "../expr-src/src/lang/language-builder.h"
#include "lang/expr-scanner.hpp"
#include <driver/evaluator.h>
#include <expr-parser.hpp>

namespace aaltitoad {
    struct language_result {
        std::map<std::string, expr::syntax_tree_t> declarations;
        std::optional<expr::syntax_tree_t> expression;
        auto get_symbol_table() -> expr::symbol_table_t {
            expr::symbol_operator op{};
            expr::evaluator e{{}, op};
            expr::symbol_table_t env{};
            for(auto& r : declarations)
                env[r.first] = e.evaluate(r.second);
            return env;
        }
    };

    class expression_driver {
    public:
        expression_driver(const std::string& environment, const std::string& unknown) : known_environment{}, unknown_environment{} {
            known_environment = parse_expressions(environment).get_symbol_table();
            unknown_environment = parse_expressions(unknown).get_symbol_table();
        }

        void evaluate(const language_result& result) {
            expr::symbol_operator op{};
            expr::evaluator e{{known_environment}, op};
            expr::symbol_table_t env{};
            for(auto& r : result.declarations)
                env[r.first] = e.evaluate(r.second);
            std::cout << " evaluated: \n";
            for(auto& s : env)
                std::cout << "\t" << s.first << " :-> " << s.second << "\n";
            if(result.expression)
                std::cout << "\t" << e.evaluate(result.expression.value()) << "\n";
        }

        void compile(const language_result& result) {
            std::cout << " result: \n";
            for(auto& r : result.declarations)
                std::cout << "\t" << r.first << " :=> " << r.second << "\n";
            if(result.expression)
                std::cout << "\t expression = " << result.expression.value() << "\n";
        }

#ifdef ENABLE_Z3
        void sat_check(const language_result& result) {
            std::cout << " sat check: \n";
            expr::z3_driver z{known_environment, unknown_environment};
            if(result.expression) {
                auto sol =  z.find_solution(result.expression.value());
                if(!sol)
                    std::cout << "\tunsat\n";
                else {
                    if(sol.value().empty())
                        std::cout << "\t already satisfied\n";
                    else
                        std::cout << "\t" << sol.value() << "\n";
                }
            }
        }
#endif

        auto parse_expressions(const std::string& s) -> language_result {
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

