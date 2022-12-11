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
#ifndef AALTITOAD_NTTA_BUILDER_H
#define AALTITOAD_NTTA_BUILDER_H
#include "ntta/tta.h"
#include "ntta/interesting_tocker.h"

namespace aaltitoad {
    struct tta_builder {
        struct edge_construction_element {
            std::string source, target;
            std::optional<std::string> guard, update; // missing: optional identifier
        };
        explicit tta_builder(expr::compiler* expression_compiler);
        auto set_name(const std::string& name) -> tta_builder&;
        auto set_starting_location(const std::string& name) -> tta_builder&;
        auto add_location(const std::string& name) -> tta_builder&;
        auto add_locations(const std::vector<std::string>& names) -> tta_builder&;
        auto add_edge(const edge_construction_element& e) -> tta_builder&;
        auto add_edges(const std::vector<edge_construction_element>& es) -> tta_builder&;
        auto build() -> tta_t;
        auto compile_guard(const std::optional<std::string>& guard) -> expr::compiler::compiled_expr_t;
        auto compile_update(const std::optional<std::string>& update) -> expr::compiler::compiled_expr_collection_t;
        auto get_name() -> std::optional<std::string>;
    private:
        expr::compiler* compiler;
        aaltitoad::tta_t::graph_builder factory;
        expr::compiler::compiled_expr_t empty_guard;
        std::optional<std::string> starting_location;
        std::optional<std::string> tta_name;
    };

    struct ntta_builder {
        struct symbol_value_pair {
            std::string name;
            expr::symbol_value_t value;
        };
        ntta_builder();
        auto add_tta(tta_builder& builder) -> ntta_builder&;
        auto add_tta(const std::string& name, tta_builder& builder) -> ntta_builder&;
        auto add_tta(const std::string& name, const tta_t& tta) -> ntta_builder&;
        auto add_symbol(const symbol_value_pair& symbol) -> ntta_builder&;
        auto add_symbols(const std::vector<symbol_value_pair>& ss) -> ntta_builder&;
        auto add_symbols(const expr::symbol_table_t& ss) -> ntta_builder&;
        auto add_external_symbol(const symbol_value_pair& symbol) -> ntta_builder&;
        auto add_external_symbols(const std::vector<symbol_value_pair>& ss) -> ntta_builder&;
        auto add_external_symbols(const expr::symbol_table_t& ss) -> ntta_builder&;
        auto build() const -> ntta_t;
        auto build_heap() const -> ntta_t*;
        auto build_with_interesting_tocker() const -> ntta_t;
        auto build_heap_with_interesting_tocker() const -> ntta_t*;

        aaltitoad::ntta_t::tta_map_t components;
        expr::symbol_table_t symbols, external_symbols;
    };
}

#endif //AALTITOAD_NTTA_BUILDER_H
