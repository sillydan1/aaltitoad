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
#ifndef AALTITOAD_QUERY_JSON_LOADER_H
#define AALTITOAD_QUERY_JSON_LOADER_H
#include <ctl_compiler.h>

namespace aaltitoad {
    auto load_query_json_file(const std::string& json_file, std::initializer_list<expr::symbol_table_ref_t> environments) -> std::vector<ctl::compiler::compiled_expr_t>;
    auto is_query_searchable(const ctl::compiler::compiled_expr_t& q) -> bool;
    auto is_query_trivial(const ctl::compiler::compiled_expr_t& q) -> bool;
}

#endif //AALTITOAD_QUERY_JSON_LOADER_H
