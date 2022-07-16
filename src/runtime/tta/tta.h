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

namespace aaltitoad {
    struct location_t {
        using graph_key_t = std::string;
        std::string identifier{ya::uuid_v4()};
    };

    struct edge_t {
        std::string identifier{ya::uuid_v4()};
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

        tta_t() : graph{}, initial_location{}, current_location{} {}
        tta_t(std::shared_ptr<graph_t> graph, location_t::graph_key_t initial_location)
         : graph{std::move(graph)}, initial_location{std::move(initial_location)},
           current_location{}
        {
            auto it = this->graph->nodes.find(this->initial_location);
            if(it == this->graph->nodes.end())
                throw std::out_of_range("no such initial location in provided TTA graph");
            current_location = it;
        }
    };

    struct ntta_t {
#ifndef NDEBUG
        using tta_map_t = std::map<std::string,tta_t>;
#else
        using tta_map_t = std::unordered_map<std::string,tta_t>;
#endif
        expr::symbol_table_t symbols;
        std::vector<expr::symbol_table_t::iterator> external_symbols;
        tta_map_t components;

        ntta_t() : symbols{}, external_symbols{}, components{}, interpreter{symbols} {}
        ntta_t(expr::symbol_table_t symbols, tta_map_t components) : symbols{std::move(symbols)}, external_symbols{}, components{std::move(components)}, interpreter{symbols} {}

        struct location_change_t {
            tta_map_t::iterator component;
            tta_t::graph_node_iterator_t new_location;
        };
        struct state_change_t {
            std::vector<location_change_t> location_changes;
            expr::symbol_table_t symbol_changes;
        };
        struct choice_t {
            tta_t::graph_edge_iterator_t edge;
            location_change_t location_change;
            expr::symbol_table_t symbol_changes;
        };
        auto tick() -> std::vector<state_change_t>;
        auto tock() const -> expr::symbol_table_t; // TODO: How do we model verification choices? - Should the injected tocker handle this?

        void apply(const state_change_t& changes);
        void apply(const expr::symbol_table_t& symbol_changes);

    private:
        static auto collect_choices(const ya::combiner_iterator_list_t<choice_t>& iterator_list) -> std::optional<state_change_t>;
        // --- management things --- //
        expr::interpreter interpreter;
    };
}

auto operator<<(std::ostream& os, const aaltitoad::ntta_t& state) -> std::ostream&;

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

    template<>
    struct hash<aaltitoad::tta_t::graph_edge_iterator_t> {
        inline auto operator()(const aaltitoad::tta_t::graph_edge_iterator_t& v) const -> size_t {
            return hash<std::string>{}(v->second.data.identifier);
        }
    };
}

#endif //AALTITOAD_TTA_H
