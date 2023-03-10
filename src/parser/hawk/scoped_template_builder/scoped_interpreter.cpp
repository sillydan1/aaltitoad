/**
 * aaltitoad - a verification engine for tick tock automata models
   Copyright (C) 2023 Asger Gitz-Johansen

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include "scoped_interpreter.h"
#include "expr-wrappers/interpreter.h"
#include "symbol_table.h"
#include <utility>

namespace aaltitoad::hawk {
    auto get_parameterized_identifier(const std::string& identifier, const expr::symbol_table_t& parameters) -> std::optional<std::string> {
        auto id = trim_copy(identifier);
        for(auto& parameter : parameters) {
            std::regex r{"\\." + parameter.first + "$"};
            std::smatch match;
            if (std::regex_search(id.cbegin(), id.cend(), match, r)) {
                auto replace = "." + expr::as_string(parameter.second);
                auto parameterized_identifier = std::regex_replace(id, r, replace);
                return {parameterized_identifier};
            }
        }
        return {};
    }

    scoped_interpreter::scoped_interpreter(std::initializer_list<std::reference_wrapper<expr::symbol_table_t>> environments, const std::string& prefix)
     : public_result{}, parameters{}, identifier_prefix{prefix} {
    }

    void scoped_interpreter::add_parameter(const std::string &key, const expr::symbol_value_t &value) {
        parameters[key] = value;
    }

    auto scoped_interpreter::parse(const std::string &expression) -> expr::symbol_value_t { 
        // TODO: implement this
    }

    auto scoped_interpreter::parse_table(const std::string &expression) -> expr::symbol_table_t { 
        // TODO: implement this
    }
/* Old functionality:
    void scoped_interpreter::add_tree(const std::string& identifier, const expr::syntax_tree_t& tree) {
        auto id = identifier;
        auto parameterized_identifier = get_parameterized_identifier(id, parameters);
        if(parameterized_identifier.has_value())
            id = parameterized_identifier.value();
        
        expr::interpreter::add_tree(identifier_prefix + id, tree);
    }

    void scoped_interpreter::add_tree(const std::string& access_modifier, const std::string& identifier, const expr::syntax_tree_t& tree) {
        auto id = identifier;
        auto parameterized_identifier = get_parameterized_identifier(id, parameters);
        if(parameterized_identifier.has_value())
            id = parameterized_identifier.value();

        if(lower_case(access_modifier) == "public")
            public_result[id] = evaluate(tree);
        else
            expr::interpreter::add_tree(access_modifier, identifier_prefix + id, tree);
    }

    auto scoped_interpreter::get_symbol(const std::string& identifier) -> expr::syntax_tree_t {
        auto env_it = parameters.find(identifier);
        if(env_it != parameters.end())
            return expr::syntax_tree_t{env_it->second};
        return expr::interpreter::get_symbol(identifier);
    }
*/
    scoped_compiler::scoped_compiler(const expr::symbol_table_t& local_symbols, const expr::symbol_table_t& parameters, const std::string& local_prefix, const std::initializer_list<std::reference_wrapper<expr::symbol_table_t>>& environments)
     : local_symbols{local_symbols}, parameters{parameters}, local_prefix{local_prefix} {

    }

    auto scoped_compiler::parse(const std::string &expression) -> language_result {
        // TODO: implement this!
    }

    auto scoped_compiler::get_localized_symbols() -> expr::symbol_table_t {
        expr::symbol_table_t localized_symbols{};
        for(auto& s : local_symbols)
            localized_symbols[s.first] = s.second;
        return localized_symbols;
    }
/*
    auto scoped_compiler::get_symbol(const std::string& identifier) -> expr::syntax_tree_t {
        // are we looking up a parameterized identifier?
        // e.g. guard: "somevar.foo > bar" => "somevar.30 > bar" where 'foo' is a parameter with the argument 30
        auto id = identifier;
        auto parameterized_identifier = get_parameterized_identifier(id, parameters);
        if(parameterized_identifier.has_value())
            id = parameterized_identifier.value();

        // convert local identifiers to flattened identifiers
        // e.g. guard "foo > bar" => "Main.Instance.foo > bar" where 'foo' is a local symbol
        auto local_it = local_symbols.find(local_prefix + id);
        if(local_it != local_symbols.end())
            return expr::syntax_tree_t{expr::identifier_t{local_prefix + id}};

        // e.g. guard: "foo > bar " => "30 > bar" where 'foo' is a parameter with the argument 30
        auto param_it = parameters.find(id);
        if(param_it != parameters.end())
            return expr::syntax_tree_t{param_it->second};

        return compiler::get_symbol(id);
    }

    void scoped_compiler::add_tree(const std::string& identifier, const expr::syntax_tree_t& tree) {
        auto id = identifier;
        auto parameterized_identifier = get_parameterized_identifier(id, parameters);
        if(parameterized_identifier.has_value())
            id = parameterized_identifier.value();

        auto local_it = local_symbols.find(local_prefix + id);
        if(local_it != local_symbols.end()) {
            expr::compiler::add_tree(local_prefix + id, tree);
            return;
        }

        expr::compiler::add_tree(id, tree);
    }
*/
}
