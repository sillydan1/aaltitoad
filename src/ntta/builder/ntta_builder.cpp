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
#include <util/exceptions/parse_error.h>
#include "expr-wrappers/interpreter.h"
#include "ntta/interesting_tocker.h"
#include "ntta_builder.h"
#include "symbol_table.h"

namespace aaltitoad {
    tta_builder::tta_builder(expression_driver* expression_compiler)
     : compiler{expression_compiler}, factory{}, empty_guard{}, starting_location{}
    {
        empty_guard = compiler->parse_guard("");
    }

    auto tta_builder::get_name() -> std::optional<std::string> {
        return tta_name;
    }

    auto tta_builder::set_name(const std::string& name) -> tta_builder& {
        tta_name = name;
        return *this;
    }

    auto tta_builder::set_starting_location(const std::string& name) -> tta_builder& {
        starting_location = name;
        return *this;
    }

    auto tta_builder::add_location(const std::string& name) -> tta_builder& {
        factory.add_node({name});
        return *this;
    }

    auto tta_builder::add_locations(const std::vector<std::string>& names) -> tta_builder& {
        for(auto& name : names)
            add_location(name);
        return *this;
    }

    auto tta_builder::add_edge(const edge_construction_element& e) -> tta_builder& {
        factory.add_edge(e.source, e.target, {.guard=compile_guard(e.guard), .updates=compile_update(e.update)});
        return *this;
    }

    auto tta_builder::add_edges(const std::vector<edge_construction_element>& es) -> tta_builder& {
        for(auto& e : es)
            add_edge(e);
        return *this;
    }

    auto tta_builder::build() -> tta_t {
        if(!starting_location.has_value())
            throw std::logic_error("no starting location provided");
        if(!factory.is_valid()) {
            std::stringstream ss{};
            auto sep = "";
            for(auto& ie : factory.get_invalid_elements()) {
                ss << sep << "[elem](" << ie << ")";
                sep = ",";
            }
            throw std::logic_error("invalid tta_t graph, affected elements: " + ss.str());
        }
        return { std::move(factory.build_heap()), starting_location.value() };
    }

    auto tta_builder::compile_guard(const std::optional<std::string>& guard) -> expr::syntax_tree_t {
        if(!guard.has_value())
            return empty_guard;
        if(guard.value().empty())
            return empty_guard;
        auto result = compiler->parse(guard.value());
        if(!result.expression.has_value())
            throw new std::logic_error("guard is not an expression");
        return result.expression.value();
    }

    auto tta_builder::compile_update(const std::optional<std::string>& update) -> expr::syntax_tree_collection_t {
        if(!update.has_value())
            return {};
        return compiler->parse(update.value()).declarations;
    }

    ntta_builder::ntta_builder() : components{}, symbols{} {

    }

    auto ntta_builder::add_tta(tta_builder& builder) -> ntta_builder& {
        if(!builder.get_name().has_value())
            throw parse_error("cannot construct ntta: tta builder does not have a name set");
        components[builder.get_name().value()] = builder.build();
        return *this;
    }

    auto ntta_builder::add_tta(const std::string& name, tta_builder& builder) -> ntta_builder& {
        components[name] = builder.build();
        return *this;
    }

    auto ntta_builder::add_tta(const std::string& name, const tta_t& builder) -> ntta_builder& {
        components[name] = builder;
        return *this;
    }

    auto ntta_builder::add_symbol(const symbol_value_pair& symbol) -> ntta_builder& {
        symbols[symbol.name] = symbol.value;
        return *this;
    }

    auto ntta_builder::add_symbols(const std::vector<symbol_value_pair>& ss) -> ntta_builder& {
        for(auto& s : ss)
            add_symbol(s);
        return *this;
    }

    auto ntta_builder::add_symbols(const expr::symbol_table_t& ss) -> ntta_builder& {
        symbols += ss;
        return *this;
    }

    auto ntta_builder::add_external_symbol(const aaltitoad::ntta_builder::symbol_value_pair &symbol) -> ntta_builder& {
        external_symbols[symbol.name] = symbol.value;
        return *this;
    }

    auto ntta_builder::add_external_symbols(const std::vector<symbol_value_pair> &ss) -> ntta_builder& {
        for(auto& s : ss)
            add_external_symbol(s);
        return *this;
    }

    auto ntta_builder::add_external_symbols(const expr::symbol_table_t &ss) -> ntta_builder& {
        for(auto& s : ss)
            add_external_symbol({s.first, s.second});
        return *this;
    }

    auto ntta_builder::build() const -> ntta_t {
        return aaltitoad::ntta_t{symbols, external_symbols, components};
    }

    auto ntta_builder::build_heap() const -> ntta_t* {
        return new aaltitoad::ntta_t{symbols, external_symbols, components};
    }

    auto ntta_builder::build_with_interesting_tocker() const -> ntta_t {
        return build().add_tocker(std::make_shared<aaltitoad::interesting_tocker>());
    }

    auto ntta_builder::build_heap_with_interesting_tocker() const -> ntta_t* {
        auto* n = build_heap();
        n->add_tocker(std::make_shared<aaltitoad::interesting_tocker>());
        return n;
    }
}
