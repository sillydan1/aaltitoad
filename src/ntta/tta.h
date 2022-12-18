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
#ifndef AALTITOAD_TTA_H
#define AALTITOAD_TTA_H
#include <string>
#include <graph>
#include <drivers/compiler.h>
#include <drivers/interpreter.h>
#include <symbol_table.h>
#include <hashcombine>
#include <utility>
#include <uuid>
#include <permutation>
#include <set>
#include <future>

namespace aaltitoad {
    struct location_t {
        using graph_key_t = std::string;
        std::string identifier{ya::uuid_v4_custom("L", "")};
    };

    struct edge_t {
        std::string identifier{ya::uuid_v4_custom("E", "")};
        expr::compiler::compiled_expr_t guard{};
        expr::compiler::compiled_expr_collection_t updates{};
        auto operator==(const edge_t& other) const -> bool {
            return identifier == other.identifier;
        }
    };
}

namespace std {
    template<>
    struct hash<aaltitoad::edge_t> {
        inline auto operator()(const aaltitoad::edge_t& v) const -> size_t {
            return std::hash<std::string>{}(v.identifier);
        }
    };
}

namespace aaltitoad {
    struct tta_t {
        using graph_builder = ya::graph_builder<location_t, edge_t, location_t::graph_key_t>;
        using graph_t = ya::graph<location_t, edge_t, location_t::graph_key_t>;
        using graph_node_iterator_t = ya::node_refference<location_t, edge_t, location_t::graph_key_t>;
        using graph_edge_iterator_t = ya::edge_refference<location_t, edge_t, location_t::graph_key_t>;
        std::shared_ptr<graph_t> graph;
        location_t::graph_key_t initial_location;
        graph_node_iterator_t current_location;

        tta_t() : graph{}, initial_location{}, current_location{graph->nodes.end()} {}
        tta_t(std::shared_ptr<graph_t> graph, location_t::graph_key_t initial_location)
                : initial_location{std::move(initial_location)},
                  current_location{graph->nodes.end()}, graph{std::move(graph)}
        {
            auto it = this->graph->nodes.find(this->initial_location);
            if(it == this->graph->nodes.end())
                throw std::out_of_range(this->initial_location + ": no such initial location in provided TTA graph");
            current_location = it;
        }
    };

    struct tocker_t;

    struct ntta_t {
#ifndef NDEBUG
        using tta_map_t = std::map<std::string,tta_t>;
#else
        using tta_map_t = std::unordered_map<std::string,tta_t>;
#endif
        struct location_change_t {
            tta_map_t::iterator component;
            tta_t::graph_node_iterator_t new_location;
        };
        struct choice_t {
            tta_t::graph_edge_iterator_t edge;
            location_change_t location_change;
            expr::symbol_table_t symbol_changes;
        };
        struct state_change_t {
            std::vector<location_change_t> location_changes;
            expr::symbol_table_t symbol_changes;
            auto operator+=(const choice_t&) -> state_change_t&;
        };

        std::vector<std::shared_ptr<tocker_t>> tockers;
        expr::symbol_table_t symbols;
        expr::symbol_table_t external_symbols;
        tta_map_t components;

        ntta_t() : symbols{}, external_symbols{}, components{} {}
        ntta_t(expr::symbol_table_t symbols, tta_map_t components)
         : symbols{std::move(symbols)}, external_symbols{}, components{std::move(components)} {}
        ntta_t(expr::symbol_table_t symbols, expr::symbol_table_t external_symbols, tta_map_t components)
         : symbols{std::move(symbols)}, external_symbols{std::move(external_symbols)}, components{std::move(components)} {}

        auto tick() -> std::vector<state_change_t>;
        auto tock() const -> std::vector<expr::symbol_table_t>;
        auto add_tocker(const std::shared_ptr<tocker_t>& tocker) -> ntta_t&;
        void apply(const state_change_t& changes);
        void apply(const expr::symbol_table_t& external_symbol_changes);
        void apply(const std::vector<expr::symbol_table_t>& external_symbol_change_list);
        void apply_internal(const expr::symbol_table_t& symbol_changes);
        auto to_string() const -> std::string;
    private:
        class tick_resolver {
        public:
            using set = std::set<std::string>;
            using solution_keys = std::vector<std::set<std::string>>;
            using graph_type = ya::graph<tta_t::graph_edge_iterator_t, uint32_t, std::string>;
            using graph_type_builder = ya::graph_builder<tta_t::graph_edge_iterator_t, uint32_t, std::string>;
            struct choice_dependency_problem {
                graph_type dependency_graph;
                std::unordered_map<std::string, choice_t> choices;
            };

            explicit tick_resolver(const graph_type& G);
            auto solve() -> solution_keys;
        private:
            void solve_recursive(solution_keys& S, const set& a);
            auto get_neighbors(const std::string& node_key) -> set;
            auto get_all_neighbors(const set& node_keys) -> set;

            const graph_type& G;
            set N;
        };
        auto calculate_edge_dependency_graph() -> tick_resolver::choice_dependency_problem;
        auto should_create_dependency_edge(const tta_t::graph_edge_iterator_t& e1, const tta_t::graph_edge_iterator_t& e2, expr::interpreter& i) const -> bool;
        static auto eval_updates(expr::interpreter& i, const expr::compiler::compiled_expr_collection_t& t) -> expr::symbol_table_t;
        static auto eval_guard(expr::interpreter& i, const expr::compiler::compiled_expr_t& e) -> expr::symbol_value_t;
    };

    struct tocker_t {
        [[nodiscard]] virtual auto tock(const ntta_t& state) -> std::vector<expr::symbol_table_t> = 0;
        [[nodiscard]] virtual auto get_name() -> std::string { return "tocker"; };
        virtual ~tocker_t() = default;
    };
}

auto operator<<(std::ostream& os, const aaltitoad::ntta_t& state) -> std::ostream&;
auto operator+(const aaltitoad::ntta_t& state, const aaltitoad::ntta_t::state_change_t& change) -> aaltitoad::ntta_t;
auto operator+(const aaltitoad::ntta_t& state, const expr::symbol_table_t& external_symbol_changes) -> aaltitoad::ntta_t;
auto operator==(const aaltitoad::ntta_t& a, const aaltitoad::ntta_t& b) -> bool;

namespace std {
    template<>
    struct hash<aaltitoad::tta_t> {
        inline auto operator()(const aaltitoad::tta_t& v) const -> size_t {
            return std::hash<aaltitoad::location_t::graph_key_t>{}(v.current_location->first);
        }
    };

    template<>
    struct hash<aaltitoad::ntta_t::tta_map_t> {
        inline auto operator()(const aaltitoad::ntta_t::tta_map_t& v) const -> size_t {
            size_t result{};
            for(auto& t : v) {
                result = ya::hash_combine(result, t.first);
                result = ya::hash_combine(result, t.second);
            }
            return result;
        }
    };

    template<>
    struct hash<aaltitoad::ntta_t> {
        inline auto operator()(const aaltitoad::ntta_t& v) const -> size_t {
            return ya::hash_combine(v.symbols, v.components);
        }
    };
}

#endif //AALTITOAD_TTA_H
