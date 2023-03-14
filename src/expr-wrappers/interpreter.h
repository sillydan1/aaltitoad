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

// TODO: Extract implementations into a cpp file
namespace expr {
    using syntax_tree_collection_t = std::map<std::string, expr::syntax_tree_t>;
}

auto operator<<(std::ostream& os, const expr::syntax_tree_collection_t& c) -> std::ostream&;

namespace aaltitoad {
    class expression_driver {
    public:
        struct language_result {
            expr::syntax_tree_collection_t declarations;
            std::optional<expr::syntax_tree_t> expression;
            auto get_symbol_table() -> expr::symbol_table_t; // NOTE: no environment context available here
            auto get_symbol_value() -> expr::symbol_value_t;
        };
        expression_driver(); // TODO: stop copying the environments, they should be reference_wrapper<const symbol_table_t> instead
        expression_driver(const expr::symbol_table_t& known);
        expression_driver(const expr::symbol_table_t& known, const expr::symbol_table_t& unknown);
        virtual ~expression_driver();

        auto evaluate(const expr::syntax_tree_collection_t& declarations) -> expr::symbol_table_t;

        auto evaluate(const expr::syntax_tree_t& expression) -> expr::symbol_value_t;
        auto sat_check(const expr::syntax_tree_t& expression) -> expr::symbol_table_t;
        auto parse_guard(const std::string& expression) -> expr::syntax_tree_t;
        virtual auto parse(const std::string& s) -> language_result;
    public:
        expr::symbol_table_t known_environment{};
        expr::symbol_table_t unknown_environment{};
    };
}

#endif

