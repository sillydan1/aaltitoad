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
#ifndef AALTITOAD_INTERESTING_TOCKER_H
#define AALTITOAD_INTERESTING_TOCKER_H
#include "expr-wrappers/interpreter.h"
#include "tta.h"
#include <unordered_map>

namespace aaltitoad {
    class interesting_tocker : public tocker_t {
    public:
        [[nodiscard]] auto tock(const ntta_t& state) -> std::vector<expr::symbol_table_t> override;
        [[nodiscard]] auto get_name() -> std::string override;
        ~interesting_tocker() override = default;
    private:
        [[nodiscard]] auto contains_timer_variables(const expr::syntax_tree_t& tree, const expr::symbol_table_t& symbols) const -> bool;
        [[nodiscard]] auto contains_external_variables(const expr::syntax_tree_t& tree, const expr::symbol_table_t& symbols) const -> bool;
        static auto find_solution(expression_driver& d, const ya::combiner_iterator_list_t<expr::syntax_tree_t>& elements) -> std::optional<expr::symbol_table_t>;
        std::unordered_map<std::string, tta_t::graph_edge_iterator_t> edge_cache;
    };
}

#endif //AALTITOAD_INTERESTING_TOCKER_H
