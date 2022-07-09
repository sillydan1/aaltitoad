#ifndef AALTITOAD_TTA_H
#define AALTITOAD_TTA_H
#include <string>
#include <graph>
#include <drivers/compiler.h>
#include <drivers/interpreter.h>
#include <symbol_table.h>
#include <hashcombine>
#include <uuid>

namespace aaltitoad {
    struct location_t {
        using graph_key_t = std::string;
        std::string identifier{ya::uuid_v4()};
    };

    struct edge_t {
        std::string identifier{ya::uuid_v4()};
        expr::compiler::compiled_expr_t guard{};
        expr::compiler::compiled_expr_collection_t updates{};
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
        using graph_t = ya::graph<location_t, edge_t, location_t::graph_key_t>;
        using graph_node_iterator_t = ya::node_refference<location_t, edge_t, location_t::graph_key_t>;
        std::shared_ptr<graph_t> graph;
        location_t::graph_key_t initial_location;
        graph_node_iterator_t current_location;

        tta_t(const std::shared_ptr<graph_t>& graph, const location_t::graph_key_t& initial_location)
         : graph{graph}, initial_location{initial_location},
           current_location{}
        {
            auto it = this->graph->nodes.find(initial_location);
            if(it == this->graph->nodes.end())
                throw std::out_of_range("No such initial location in provided TTA graph");
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

        struct state_change_t {
            struct location_change_t {
                tta_map_t::iterator component;
                tta_t::graph_node_iterator_t new_location;
            };
            std::vector<location_change_t> location_changes;
            expr::symbol_table_t symbol_changes;
        };
        auto tick() const -> state_change_t; // TODO: How do we model choices?
        void tick(const state_change_t& changes);
        auto tock() const -> expr::symbol_table_t; // TODO: How do we model choices?
        void tock(const expr::symbol_table_t& symbol_changes);

    private:
        // --- management things --- //
        expr::interpreter interpreter;
    };
}

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
