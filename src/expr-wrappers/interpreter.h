#ifndef AALTITOAD_EXPR_WRAPPER_INTERPRETER_H
#define AALTITOAD_EXPR_WRAPPER_INTERPRETER_H
#include <symbol_table.h>
#include <driver/z3/z3-driver.h>
#include "../expr-src/src/lang/ast-factory.h"
#include "../expr-src/src/lang/language-builder.h"
#include "lang/expr-scanner.hpp"
#include <driver/evaluator.h>
#include <expr-parser.hpp>

namespace expr {
    using syntax_tree_collection_t = std::map<std::string, expr::syntax_tree_t>;
}

namespace aaltitoad {
    struct language_result {
        expr::syntax_tree_collection_t declarations;
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
            known_environment = parse(environment).get_symbol_table();
            unknown_environment = parse(unknown).get_symbol_table();
        }
        
        expression_driver(const expr::symbol_table_t& known, const expr::symbol_table_t& unknown) : known_environment{known}, unknown_environment{unknown} {}

        auto evaluate(const language_result& result) -> expr::symbol_table_t {
            expr::symbol_operator op{};
            expr::evaluator e{{known_environment}, op}; // TODO: environment(s) should be injected directly instead of through the ctor
            expr::symbol_table_t env{};
            for(auto& r : result.declarations)
                env[r.first] = e.evaluate(r.second);
            return env;
        }

        auto sat_check(const expr::syntax_tree_t& expression) -> expr::symbol_table_t { // TODO: This could return an optional instead
            expr::z3_driver z{known_environment, unknown_environment};
            auto sol = z.find_solution(expression);
            if(sol)
                return sol.value();
            return {};
        }

        auto parse(const std::string& s) -> language_result {
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
        expr::symbol_table_t known_environment{}; // TODO: These environments are only used for z3
        expr::symbol_table_t unknown_environment{};
    };
}

#endif

