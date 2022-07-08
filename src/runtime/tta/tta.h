#ifndef AALTITOAD_TTA_H
#define AALTITOAD_TTA_H
#include <string>
#include <graph>

namespace aaltitoad {
    struct location_t {
        using key_t = std::string;
        std::string identifier;
    };

    struct edge_t {
        // guard
        // update
        // clock reset-set
    };

    struct tta_t {
        // TODO: internal variables
        // TODO: external variables (including clocks)
        ya::graph<location_t, edge_t, location_t::key_t> graph;

        // --- state --- //
        // TODO: initial location
        // TODO: current location
        // TODO: current variable valuation (implicitly defined in symboltable_t's)

        // --- state manipulation --- //
        // TODO: Tick (non-const) - modify the state
        // TODO: Tick (const) - calculate changes
        // TODO: Tock (non-const) - modify the state
        // TODO: Tock (const) - calculate changes
    };
}

#endif //AALTITOAD_TTA_H
