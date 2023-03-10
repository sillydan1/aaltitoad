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
#include "expr-wrappers/interpreter.h"
#include <symbol_table.h>

namespace aaltitoad::hawk {
    struct scoped_interpreter {
        scoped_interpreter(std::initializer_list<std::reference_wrapper<expr::symbol_table_t>> environments, const std::string& prefix);
        auto parse(const std::string& expression) -> expr::symbol_value_t;
        auto parse_table(const std::string& expression) -> expr::symbol_table_t; // TODO: Rename
        void add_parameter(const std::string& key, const expr::symbol_value_t& value);
        expr::symbol_table_t public_result;
        expr::symbol_table_t parameters{};
        std::string identifier_prefix{};
    };

    struct scoped_compiler : public expression_driver { // TODO: This should just be part of the expression_driver class itself
        scoped_compiler(const expr::symbol_table_t& local_symbols, const expr::symbol_table_t& parameters, const std::string& local_prefix, const std::initializer_list<std::reference_wrapper<expr::symbol_table_t>>& environments);
        auto parse(const std::string& expression) -> language_result override;
        auto get_localized_symbols() -> expr::symbol_table_t;

    private:
        expr::symbol_table_t local_symbols;
        expr::symbol_table_t parameters;
        std::string local_prefix;
    };
}

#endif //AALTITOAD_SCOPED_INTERPRETER_H
