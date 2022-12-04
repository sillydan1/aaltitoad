#include "scoped_interpreter.h"

#include <utility>

namespace aaltitoad::hawk {
    auto get_parameterized_identifier(const std::string& prefix, const std::string& identifier, const expr::symbol_table_t& parameters) -> std::optional<std::string> {
        auto id = trim_copy(identifier);
        for(auto& parameter : parameters) {
            std::regex r{"\\." + parameter.first + "$"};
            std::smatch match;
            if (std::regex_search(id.cbegin(), id.cend(), match, r)) {
                auto replace = "." + expr::as_string(parameter.second);
                auto parameterized_identifier = std::regex_replace(id, r, replace);
                return {prefix + parameterized_identifier};
            }
        }
        return {};
    }

    scoped_interpreter::scoped_interpreter(std::initializer_list<expr::symbol_table_ref_t> environments)
            : expr::interpreter{environments} {

    }

    void scoped_interpreter::add_tree(const std::string& identifier, const expr::syntax_tree_t& tree) {
        auto id = identifier;
        auto parameterized_identifier = get_parameterized_identifier(identifier_prefix, id, parameters);
        if(parameterized_identifier.has_value())
            id = parameterized_identifier.value();

        spdlog::trace("adding variable {0}", id);
        expr::interpreter::add_tree(id, tree);
    }

    void scoped_interpreter::add_tree(const std::string& access_modifier, const std::string& identifier, const expr::syntax_tree_t& tree) {
        auto id = identifier;
        auto parameterized_identifier = get_parameterized_identifier(identifier_prefix, id, parameters);
        if(parameterized_identifier.has_value())
            id = parameterized_identifier.value();

        spdlog::trace("adding variable {0} {1}", access_modifier, id);
        if(lower_case(access_modifier) == "public")
            public_result[id] = evaluate(tree);
        else
            expr::interpreter::add_tree(access_modifier, id, tree);
    }

    auto scoped_interpreter::get_symbol(const std::string& identifier) -> expr::syntax_tree_t {
        auto env_it = parameters.find(identifier);
        if(env_it != parameters.end())
            return expr::syntax_tree_t{env_it->second};
        return expr::interpreter::get_symbol(identifier);
    }

    scoped_compiler::scoped_compiler(expr::symbol_table_t local_symbols, expr::symbol_table_t parameters, std::string local_prefix, std::initializer_list<expr::symbol_table_ref_t> environments)
     : expr::compiler{environments}, local_symbols{std::move(local_symbols)}, parameters{std::move(parameters)}, local_prefix{std::move(local_prefix)} {

    }

    auto scoped_compiler::get_symbol(const std::string& identifier) -> expr::syntax_tree_t {
        // are we looking up a parameterized identifier?
        // e.g. guard: "somevar.foo > bar" => "somevar.30 > bar" where 'foo' is a parameter with the argument 30
        auto id = identifier;
        auto parameterized_identifier = get_parameterized_identifier(local_prefix, id, parameters);
        if(parameterized_identifier.has_value())
            id = parameterized_identifier.value();

        // convert local identifiers to flattened identifiers
        // e.g. guard "foo > bar" => "Main.Instance.foo > bar" where 'foo' is a local symbol
        auto local_it = local_symbols.find(id);
        if(local_it != local_symbols.end())
            return expr::syntax_tree_t{expr::identifier_t{id}};

        // e.g. guard: "foo > bar " => "30 > bar" where 'foo' is a parameter with the argument 30
        auto param_it = parameters.find(id);
        if(param_it != parameters.end())
            return expr::syntax_tree_t{param_it->second};

        return compiler::get_symbol(id);
    }

    auto scoped_compiler::get_localized_symbols() -> expr::symbol_table_t {
        expr::symbol_table_t localized_symbols{};
        for(auto& s : local_symbols)
            localized_symbols[s.first] = s.second;
        return localized_symbols;
    }
}
