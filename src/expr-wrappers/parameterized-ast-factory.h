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
#ifndef PARAMETERIZED_AST_FACTORY_H
#define PARAMETERIZED_AST_FACTORY_H
#include "symbol_table.h"
#include <expr-lang/ast-factory.h>

namespace aaltitoad {
    class parameterized_ast_factory : public expr::ast_factory {
    public:
        parameterized_ast_factory(const std::string& scope_prefix, const expr::symbol_table_t& paramargs, const std::vector<std::string>& local_names);
        ~parameterized_ast_factory() override;
        // Builds identifier-nodes that are parameterized.
        // e.g.: parameters: foo :-> 32 and declaration 'private bar := 2' in template instance 'Main.Baz'
        // then expression 'bar + foo' would be converted to 'Main.Baz.bar + 32'
        auto build_identifier(const std::string& identifier) -> expr::syntax_tree_t override;
    private:
        auto get_parameterized_identifier(const std::string& identifier) const -> std::string;
        std::string scope_prefix;
        std::vector<std::string> local_identifiers;
        const expr::symbol_table_t& parameter_arguments;
    };
}

#endif

