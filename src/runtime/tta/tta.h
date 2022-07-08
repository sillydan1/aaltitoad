#ifndef AALTITOAD_TTA_H
#define AALTITOAD_TTA_H
#include <string>
#include <graph>
#include <drivers/compiler.h>
#include <drivers/interpreter.h>

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
        // --- syntax --- //
        // TODO: internal variables
        // TODO: external variables (includes clocks)
        ya::graph<location_t, edge_t, location_t::graph_key_t> graph;

        // --- state --- //
        // TODO: initial location
        // TODO: current location
        // TODO: current variable valuation (implicitly defined in symboltable_t's)

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
