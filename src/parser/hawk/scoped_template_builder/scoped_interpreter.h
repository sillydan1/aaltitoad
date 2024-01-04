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
#ifndef AALTITOAD_SCOPED_INTERPRETER_H
#define AALTITOAD_SCOPED_INTERPRETER_H
#include "driver/evaluator.h"
#include "expr-wrappers/interpreter.h"
#include <symbol_table.h>

namespace aaltitoad::hawk {
    class parameterizer {
    public:
        auto get_parameters() const -> expr::symbol_table_t;
        auto get_prefix() const -> std::string;
        void add_parameter(const std::string& key, const expr::symbol_value_t& value);
    protected:
        parameterizer(const expr::symbol_table_t& params, const std::string& identifier_prefix);
        virtual ~parameterizer();
        auto get_parameterized_identifier(const std::string& identifier) const -> std::string;
        auto parse_with_local_identifiers(const std::string& expression, const std::vector<std::string>& local_identifiers) -> expr::declaration_tree_builder::result_t;
        expr::symbol_table_t parameters;
        std::string identifier_prefix;
    };

    struct scoped_interpreter : public expr::evaluator, parameterizer {
        scoped_interpreter(const expr::symbol_table_ref_collection_t& environments, const std::string& prefix);
        ~scoped_interpreter() override = default;
        auto parse_raw(const std::string& expression) -> expr::symbol_value_t;
        auto parse_declarations(const std::string& expression) -> expr::symbol_table_t;
        auto find(const std::string& identifier) const -> expr::symbol_table_t::const_iterator override;
        auto get_local_identifiers() -> std::vector<std::string>;
    private:
        std::vector<std::string> local_identifiers;
    };

    struct scoped_compiler : public expression_driver, parameterizer {
        scoped_compiler(const std::vector<std::string>& local_identifiers, const expr::symbol_table_t& parameters, const std::string& local_prefix, const std::initializer_list<std::reference_wrapper<expr::symbol_table_t>>& environments);
        auto parse(const std::string& expression) -> language_result override;
    private:
        std::vector<std::string> local_identifiers;
    };
}

#endif //AALTITOAD_SCOPED_INTERPRETER_H
