#ifndef PARAMETERIZED_EXPR_EVALUATOR_H
#define PARAMETERIZED_EXPR_EVALUATOR_H

#include "expr-wrappers/interpreter.h"
#include "symbol_table.h"
namespace aaltitoad {
    class parameterized_expr_evaluator {
    public:
        struct language_result {
            expr::syntax_tree_collection_t declarations;
            std::optional<expr::syntax_tree_t> expression;
        };
        parameterized_expr_evaluator(const std::unordered_map<std::string, expr::symbol_value_t>& paramargs);
        virtual ~parameterized_expr_evaluator() = default;
        auto parse(const std::string& expression) -> language_result;
        auto eval(const expr::syntax_tree_collection_t& declarations) -> expr::symbol_table_t; 
        auto eval(const expr::syntax_tree_t& expression) -> expr::symbol_value_t;
    };
}

#endif // !PARAMETERIZED_EXPR_EVALUATOR_H
