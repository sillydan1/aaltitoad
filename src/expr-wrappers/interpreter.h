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
        // TODO: we shouldn't copy the environments!
        expression_driver(const expr::symbol_table_t& known, const expr::symbol_table_t& unknown) : known_environment{known}, unknown_environment{unknown} {}
        expression_driver(const expr::symbol_table_t& known) : known_environment{known}, unknown_environment{} {}
        expression_driver() : known_environment{}, unknown_environment{} {}

        auto evaluate(const expr::syntax_tree_collection_t& declarations) -> expr::symbol_table_t {
            expr::symbol_operator op{};
            expr::evaluator e{{known_environment}, op};// TODO: environment(s) should be injected directly instead of through the ctor
            expr::symbol_table_t result{};
            for(auto& decl : declarations)
                result[decl.first] = e.evaluate(decl.second);
            return result;
        }

        auto evaluate(const expr::syntax_tree_t& expression) -> expr::symbol_value_t {
            expr::symbol_operator op{};
            expr::evaluator e{{known_environment}, op};// TODO: environment(s) should be injected directly instead of through the ctor
            return e.evaluate(expression);
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

    class ctl_interpreter {
    public:
        ctl_interpreter(const expr::symbol_table_t& env1, const expr::symbol_table_t& env2);
        ctl_interpreter(const expr::symbol_table_t& env1); 
        ctl_interpreter(const std::initializer_list<std::reference_wrapper<expr::symbol_table_t>>& environments) : environments{environments} {}
        auto compile(const std::string& expression) -> ctl::syntax_tree_t {
            // TODO: implement this
            throw std::logic_error("not implemented yet");
        }
    private:
        std::vector<std::reference_wrapper<expr::symbol_table_t>> environments;
    };
}

#endif

