#include "ntta_builder.h"

namespace aaltitoad {
    tta_builder::tta_builder(expr::symbol_table_t& symbols) : compiler{symbols}, symbols{symbols}, factory{}, empty_guard{}, starting_location{} {
        empty_guard = compile_guard("");
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
        return {std::move(factory.build_heap()), starting_location.value()};
    }
    auto tta_builder::compile_guard(const std::optional<std::string>& guard) -> expr::compiler::compiled_expr_t {
        if(!guard.has_value())
            return empty_guard;
        compiler.trees = {};
        compiler.parse(guard.value());
        return compiler.trees["expression_result"];
    }
    auto tta_builder::compile_update(const std::optional<std::string>& update) -> expr::compiler::compiled_expr_collection_t {
        if(!update.has_value())
            return {};
        compiler.trees = {};
        compiler.parse(update.value());
        return compiler.trees;
    }

    ntta_builder::ntta_builder() : components{}, symbols{} {

    }
    auto ntta_builder::add_tta(const std::string& name, tta_builder& builder) -> ntta_builder& {
        components[name] = builder.build();
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
    auto ntta_builder::build() const -> ntta_t {
        return aaltitoad::ntta_t{symbols, components};
    }
    auto ntta_builder::build_with_interesting_tocker() -> ntta_t {
        return build().add_tocker(std::make_shared<aaltitoad::interesting_tocker>());
    }
}
