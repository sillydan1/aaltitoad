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
#ifndef AALTITOAD_FORWARD_REACHABILITY_H
#define AALTITOAD_FORWARD_REACHABILITY_H
#include "verification/ctl/ctl_sat.h"
#include "ntta/tta.h"
#include "traceable_multimap.h"
#include <ctl_syntax_tree.h>
#include <nlohmann/json.hpp>
#include <vector>
#include <utility>

namespace aaltitoad {
    class forward_reachability_searcher {
    public:
        using solution_t = traceable_multimap<ntta_t>::iterator_t;
        using compiled_query_t = ctl::syntax_tree_t;
        struct query_solution_t {
            compiled_query_t query;
            std::optional<solution_t> solution;
            query_solution_t(compiled_query_t query) : query{std::move(query)}, solution{} {}
        };
        using solutions_t = std::vector<query_solution_t>;
        explicit forward_reachability_searcher(const pick_strategy& strategy = pick_strategy::first);
        auto is_reachable(const ntta_t& s0, const compiled_query_t& q) -> solutions_t;
        auto is_reachable(const ntta_t& s0, const std::vector<compiled_query_t>& q) -> solutions_t;

    private:
        traceable_multimap<ntta_t> W{}, P{};
        solutions_t solutions{};
        pick_strategy strategy{};

        static auto empty_solution_set(const std::vector<compiled_query_t>& q) -> solutions_t;
        auto check_satisfactions(const solution_t& s) -> bool;
        auto count_solutions() -> size_t;
        auto get_results() -> solutions_t;
    };
}
auto operator<<(std::ostream& o, const aaltitoad::forward_reachability_searcher::solution_t& s) -> std::ostream&;
auto to_json(const aaltitoad::forward_reachability_searcher::solution_t& s) -> std::vector<nlohmann::json>;

#endif //AALTITOAD_FORWARD_REACHABILITY_H
