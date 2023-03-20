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
#include "parameterized-expr-evaluator.h"
#include "driver/evaluator.h"
#include "symbol_table.h"

namespace aaltitoad {
    parameterized_expr_evaluator::parameterized_expr_evaluator(const expr::symbol_table_ref_collection_t& environments, const expr::symbol_table_t& paramargs, const expr::symbol_operator& op) 
    : expr::evaluator{environments, op}, parameter_arguments{paramargs} {}

    parameterized_expr_evaluator::~parameterized_expr_evaluator() {

    }

    auto parameterized_expr_evaluator::find(const std::string& identifier) const -> expr::symbol_table_t::const_iterator {
        auto env_it = parameter_arguments.find(identifier);
        if(env_it != parameter_arguments.end())
            return env_it; 
        return expr::evaluator::find(identifier);
    }
}

