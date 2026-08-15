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
#include "driver/evaluator.h"
#include "expr-wrappers/interpreter.h"
#include "expr-wrappers/parameterized-ast-factory.h"
#include "language-builder.h"
#include "operations/symbol-operator.h"
#include "scoped_interpreter.h"
#include "symbol_table.h"
#include <utility>
#include <vector>

namespace aaltitoad::hawk {
    parameterizer::parameterizer(const expr::symbol_table_t& params, const std::string& identifier_prefix)
    : parameters{params}, identifier_prefix{identifier_prefix} {}

    parameterizer::~parameterizer() {}

    auto parameterizer::get_parameterized_identifier(const std::string& identifier) const -> std::string {
        auto id = trim_copy(identifier);
        for(auto& parameter : parameters) {
            std::regex r{"\\." + parameter.first + "$"};
            std::smatch match;
            if (std::regex_search(id.cbegin(), id.cend(), match, r)) {
                auto replace = "." + expr::as_string(parameter.second);
                auto parameterized_identifier = std::regex_replace(id, r, replace);
                return parameterized_identifier;
            }
        }
        return id;
    }

    auto parameterizer::parse_with_local_identifiers(const std::string& expression, const std::vector<std::string>& local_identifiers) -> expr::declaration_tree_builder::result_t {
        std::istringstream iss{expression};
        parameterized_ast_factory factory{identifier_prefix, parameters, local_identifiers};
        expr::declaration_tree_builder builder{};
        expr::scanner sc{iss, std::cerr, &factory};
        expr::parser_args pa{expression, &sc, &factory, &builder};
        expr::parser p{pa};
        if(p.parse() != 0)
            throw std::logic_error("parameterizer: unable to parse the expression(s): " + expression);
        return builder.build();
    }

    auto parameterizer::get_parameters() const -> expr::symbol_table_t {
        return parameters;
    }

    auto parameterizer::get_prefix() const -> std::string {
        return identifier_prefix;
    }

    void parameterizer::add_parameter(const std::string &key, const expr::symbol_value_t &value) {
        parameters[key] = value;
    }

    scoped_interpreter::scoped_interpreter(const expr::symbol_table_ref_collection_t& environments, const std::string& prefix)
    : expr::evaluator{environments, expr::symbol_operator{}}, parameterizer{{},prefix} {}

    auto scoped_interpreter::find(const std::string& identifier) const -> expr::symbol_table_t::const_iterator {
        auto env_it = parameters.find(identifier);
        if(env_it != parameters.end())
            return env_it; 
        return expr::evaluator::find(identifier);
    }

    // This function is used to evaluate some expression to some raw symbol value.
    // Explicitly, this is used for calculating the value of the arguments provided to instances of TTA templates
    auto scoped_interpreter::parse_raw(const std::string &expression) -> expr::symbol_value_t { 
        auto res = parse_with_local_identifiers(expression, {});
        return evaluate(res.raw_expression.value());
    }

    // This function is used to evaluate the declaration expression during TTA template instantiation
    auto scoped_interpreter::parse_declarations(const std::string &expression) -> expr::symbol_table_t { 
        auto res = parse_with_local_identifiers(expression, {});
        expr::symbol_table_t result{};
        for(auto& decl : res.declarations) {
            auto ident = get_parameterized_identifier(decl.first);
            if(decl.second.access_modifier == expr::symbol_access_modifier_t::_private) {
                local_identifiers.push_back(ident);
                ident = identifier_prefix + ident;
            }
            result[ident] = evaluate(decl.second.tree);
        }
        return result;
    }
    
    auto scoped_interpreter::get_local_identifiers() -> std::vector<std::string> {
        return local_identifiers;
    }

    scoped_compiler::scoped_compiler(const std::vector<std::string>& local_identifiers, const expr::symbol_table_t& parameters, const std::string& local_prefix, const std::initializer_list<std::reference_wrapper<expr::symbol_table_t>>& environments)
     : expression_driver{}, parameterizer{parameters, local_prefix}, local_identifiers{local_identifiers} {

    }

    auto scoped_compiler::parse(const std::string &expression) -> language_result {
        auto res = parse_with_local_identifiers(expression, local_identifiers);
        language_result result{};
        for(auto& decl : res.declarations) {
            auto ident = get_parameterized_identifier(decl.first);
            if(std::find(local_identifiers.begin(), local_identifiers.end(), ident) != local_identifiers.end())
                ident = identifier_prefix + ident;
            result.declarations[ident] = decl.second.tree;
        }
        if(res.raw_expression)
            result.expression = res.raw_expression.value();
        return result;
    }
}

