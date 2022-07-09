#ifndef AALTITOAD_TTA_H
#define AALTITOAD_TTA_H
#include <string>
#include <graph>
#include <drivers/compiler.h>
#include <drivers/interpreter.h>
#include <symbol_table.h>

namespace aaltitoad {
    struct location_t {
        using graph_key_t = std::string;
        std::string identifier;
    };

    struct edge_t {
        expr::compiler::compiled_expr_t guard;
        expr::compiler::compiled_expr_collection_t updates;
    };

    struct tta_t {
        ya::graph<location_t, edge_t, location_t::graph_key_t> graph;
        location_t::graph_key_t initial_location;
        location_t::graph_key_t current_location;
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

        // --- state manipulation --- //
        // TODO: Tick (non-const) - modify the state
        // TODO: Tick (const) - calculate changes
        // TODO: Tock (non-const) - modify the state
        // TODO: Tock (const) - calculate changes

    private:
        // --- management things --- //
        expr::interpreter interpreter;
    };
}

#endif //AALTITOAD_TTA_H
