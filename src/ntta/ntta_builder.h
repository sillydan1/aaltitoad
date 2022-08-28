#ifndef AALTITOAD_NTTA_BUILDER_H
#define AALTITOAD_NTTA_BUILDER_H
#include "tta.h"
#include "interesting_tocker.h"

namespace aaltitoad {
    struct tta_builder {
        struct edge_construction_element {
            std::string source, target;
            std::optional<std::string> guard, update; // missing: optional identifier
        };
        explicit tta_builder(expr::symbol_table_t& symbols) : compiler{symbols}, symbols{symbols}, factory{}, empty_guard{}, starting_location{} {
            empty_guard = compile_guard("");
        }
        auto set_starting_location(const std::string& name) -> tta_builder& {
            starting_location = name;
            return *this;
        }
        auto add_location(const std::string& name) -> tta_builder& {
            factory.add_node({name});
            return *this;
        }
        auto add_locations(const std::vector<std::string>& names) -> tta_builder& {
            for(auto& name : names)
                add_location(name);
            return *this;
        }
        auto add_edge(const edge_construction_element& e) -> tta_builder& {
            factory.add_edge(e.source, e.target, {.guard=compile_guard(e.guard), .updates=compile_update(e.update)});
            return *this;
        }
        auto add_edges(const std::vector<edge_construction_element>& es) -> tta_builder& {
            for(auto& e : es)
                add_edge(e);
            return *this;
        }
        auto build() -> tta_t {
            if(!starting_location.has_value())
                throw std::logic_error("no starting location provided");
            return {std::move(factory.build_heap()), starting_location.value()};
        }
        auto compile_guard(const std::optional<std::string>& guard) -> expr::compiler::compiled_expr_t {
            if(!guard.has_value())
                return empty_guard;
            compiler.trees = {};
            compiler.parse(guard.value());
            return compiler.trees["expression_result"];
        }
        auto compile_update(const std::optional<std::string>& update) -> expr::compiler::compiled_expr_collection_t {
            if(!update.has_value())
                return {};
            compiler.trees = {};
            compiler.parse(update.value());
            return compiler.trees;
        }
    private:
        expr::compiler compiler;
        expr::symbol_table_t& symbols;
        aaltitoad::tta_t::graph_builder factory;
        expr::compiler::compiled_expr_t empty_guard;
        std::optional<std::string> starting_location;
    };

    struct ntta_builder {
        struct symbol_value_pair {
            std::string name;
            expr::symbol_value_t value;
        };
        ntta_builder() : components{}, symbols{} {}
        auto add_tta(const std::string& name, tta_builder& builder) -> ntta_builder& {
            components[name] = builder.build();
            return *this;
        }
        auto add_symbol(const symbol_value_pair& symbol) -> ntta_builder& {
            symbols[symbol.name] = symbol.value;
            return *this;
        }
        auto add_symbols(const std::vector<symbol_value_pair>& ss) -> ntta_builder& {
            for(auto& s : ss)
                add_symbol(s);
            return *this;
        }
        auto build() const -> ntta_t {
            return aaltitoad::ntta_t{symbols, components};
        }
        auto build_with_interesting_tocker() -> ntta_t {
            return build().add_tocker(std::make_shared<aaltitoad::interesting_tocker>());
        }

        aaltitoad::ntta_t::tta_map_t components;
        expr::symbol_table_t symbols;
    };
}

#endif //AALTITOAD_NTTA_BUILDER_H
