#ifndef AALTITOAD_COMPONENT_H
#define AALTITOAD_COMPONENT_H
#include "location.h"
#include "edge.h"

/**
 * A component consists of:
 *  - A set of Locations, and
 *  - A set of edges from and to locations
 * */
struct component_t {
    using location_map_t = std::unordered_map<std::string, location_t>;
    std::vector<edge_t> edges;
    location_map_t locations;
    location_map_t::const_iterator initial_location;
    location_map_t::const_iterator current_location;

    auto get_enabled_edges(const symbol_map_t&) const -> std::vector<const edge_t*>;
    auto operator=(const std::string& new_location) -> component_t&;
};

#endif //AALTITOAD_COMPONENT_H
