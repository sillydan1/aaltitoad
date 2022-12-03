#include "scoped_interpreter.h"

#include <utility>

namespace aaltitoad::hawk {
    void scoped_interpreter::add_tree(const std::string& access_modifier, const std::string& identifier, const expr::syntax_tree_t& tree) {
        if(lower_case(access_modifier) == "public")
            public_result[identifier] = evaluate(tree);
        else
            expr::interpreter::add_tree(access_modifier, identifier, tree);
    }

    scoped_interpreter::scoped_interpreter(std::initializer_list<expr::symbol_table_ref_t> environments)
     : expr::interpreter{environments} {

    }

    scoped_compiler::scoped_compiler(expr::symbol_table_t local_symbols, std::string local_prefix, std::initializer_list<expr::symbol_table_ref_t> environments)
     : expr::compiler{environments}, local_symbols{std::move(local_symbols)}, local_prefix{std::move(local_prefix)} {

    }

    auto scoped_compiler::get_symbol(const std::string& identifier) -> expr::syntax_tree_t {
        auto env_it = local_symbols.find(identifier);
        if(env_it != local_symbols.end())
            return compiler::get_symbol(identifier);;
        return expr::syntax_tree_t{expr::identifier_t{local_prefix + identifier}};
    }

    auto scoped_compiler::get_localized_symbols() -> expr::symbol_table_t {
        expr::symbol_table_t localized_symbols{};
        for(auto& s : local_symbols)
            localized_symbols[local_prefix+s.first] = s.second;
        return localized_symbols;
    }
}
