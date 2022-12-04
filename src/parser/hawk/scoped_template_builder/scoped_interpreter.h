#ifndef AALTITOAD_SCOPED_INTERPRETER_H
#define AALTITOAD_SCOPED_INTERPRETER_H
#include <drivers/tree_interpreter.h>
#include <drivers/interpreter.h>

namespace aaltitoad::hawk {
    struct scoped_interpreter : public expr::interpreter {
        scoped_interpreter(std::initializer_list<expr::symbol_table_ref_t> environments);
        ~scoped_interpreter() override = default;
        void add_tree(const std::string& identifier, const expr::syntax_tree_t& tree) override;
        void add_tree(const std::string& access_modifier, const std::string& identifier, const expr::syntax_tree_t& tree) override;
        auto get_symbol(const std::string& identifier) -> expr::syntax_tree_t override;
        expr::symbol_table_t public_result;
        expr::symbol_table_t parameters{};
        std::string identifier_prefix{};
    };

    struct scoped_compiler : public expr::compiler {
        scoped_compiler(expr::symbol_table_t local_symbols, expr::symbol_table_t parameters, std::string local_prefix, std::initializer_list<expr::symbol_table_ref_t> environments);
        ~scoped_compiler() override = default;
        void add_tree(const std::string& identifier, const expr::syntax_tree_t& tree) override;
        auto get_symbol(const std::string &identifier) -> expr::syntax_tree_t override;
        auto get_localized_symbols() -> expr::symbol_table_t;

    private:
        expr::symbol_table_t local_symbols;
        expr::symbol_table_t parameters;
        std::string local_prefix;
    };
}

#endif //AALTITOAD_SCOPED_INTERPRETER_H
