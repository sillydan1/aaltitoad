#include "scoped_interpreter.h"

namespace aaltitoad::hawk {
    scoped_interpreter::scoped_interpreter(const expr::symbol_table_tree_t::_left_df_iterator& it) : expr::tree_interpreter(it) {

    }

    void scoped_interpreter::add_tree(const std::string& access_modifier, const std::string& identifier, const expr::syntax_tree_t& tree) {
        if(lower_case(access_modifier) == "public")
            public_result[identifier] = evaluate(tree);
        else
            tree_interpreter::add_tree(access_modifier, identifier, tree);
    }
}
