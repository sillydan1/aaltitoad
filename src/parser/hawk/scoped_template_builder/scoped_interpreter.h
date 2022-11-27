#ifndef AALTITOAD_SCOPED_INTERPRETER_H
#define AALTITOAD_SCOPED_INTERPRETER_H
#include <drivers/tree_interpreter.h>

namespace aaltitoad::hawk {
    struct scoped_interpreter : public expr::tree_interpreter {
        explicit scoped_interpreter(const expr::symbol_table_tree_t::_left_df_iterator& it);
        ~scoped_interpreter() override = default;
        void add_tree(const std::string& access_modifier, const std::string& identifier, const expr::syntax_tree_t& tree) override;
        expr::symbol_table_t public_result;
    };
}

#endif //AALTITOAD_SCOPED_INTERPRETER_H
