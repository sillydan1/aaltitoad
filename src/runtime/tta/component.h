#ifndef AALTITOAD_COMPONENT_H
#define AALTITOAD_COMPONENT_H
#include "update_expression.h"
#include "location.h"
#include "edge.h"

/**
 * A component consists of:
 *  - A set of Locations, and
 *  - A set of edges from and to locations
 * */
struct component_t {
    std::vector<location_t> locations;
    std::vector<edge_t> edges;
    std::vector<location_t>::const_iterator initial_location;
    std::vector<location_t>::const_iterator current_location;
    auto get_enabled_edges(const symbol_map_t&) const -> std::vector<edge_t>;
};

#endif //AALTITOAD_COMPONENT_H
