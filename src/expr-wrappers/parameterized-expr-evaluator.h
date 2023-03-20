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
#ifndef PARAMETERIZED_EXPR_EVALUATOR_H
#define PARAMETERIZED_EXPR_EVALUATOR_H
#include "driver/evaluator.h"
#include "interpreter.h"
#include "operations/symbol-operator.h"
#include "symbol_table.h"
#include <unordered_map>

namespace aaltitoad {
    class parameterized_expr_evaluator : public expr::evaluator {
    public:
        parameterized_expr_evaluator(const expr::symbol_table_ref_collection_t& environments, const expr::symbol_table_t& paramargs, const expr::symbol_operator& op); 
        ~parameterized_expr_evaluator() override;
        auto find(const std::string& identifier) const -> expr::symbol_table_t::const_iterator override;
    private:
        expr::symbol_table_t parameter_arguments;
        expr::symbol_table_ref_collection_t environments;
        expr::symbol_operator op;
    };
}

#endif // !PARAMETERIZED_EXPR_EVALUATOR_H
