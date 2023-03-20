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
#include "parameterized-ast-factory.h"
#include "ast-factory.h"

namespace aaltitoad {
    parameterized_ast_factory::parameterized_ast_factory(const std::string& scope_prefix, const expr::symbol_table_t& paramargs, const std::vector<std::string>& local_names)
    : scope_prefix{scope_prefix}, parameter_arguments{paramargs}, local_identifiers{local_names} {}

    parameterized_ast_factory::~parameterized_ast_factory() {}
    
    auto parameterized_ast_factory::get_parameterized_identifier(const std::string& identifier) const -> std::string {
        auto id = trim_copy(identifier);
        for(auto& parameter : parameter_arguments) {
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

    auto parameterized_ast_factory::build_identifier(const std::string& identifier) -> expr::syntax_tree_t {
        // Is the identifier a parameter name?
        if(parameter_arguments.contains(identifier))
            return expr::ast_factory::build_literal(parameter_arguments.at(identifier));
        // Is the identifier a locally declared variable?
        if(std::find(local_identifiers.begin(), local_identifiers.end(), identifier) != local_identifiers.end())
            return expr::ast_factory::build_identifier(scope_prefix + identifier);
        // Otherwise, default case (parameterized, just in case it is partial)
        return expr::ast_factory::build_identifier(get_parameterized_identifier(identifier));
    }
}

