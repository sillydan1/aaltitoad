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
    location_map_t locations;
    edge_list_t edges;
    location_map_t::key_type initial_location;
    location_map_t::key_type current_location;

    component_t(location_map_t&& locations, edge_list_t&& edges, const location_map_t::key_type& _initial_location) :
        edges{edges}, locations{locations}, initial_location{}, current_location{} {
        auto initial_location_it = locations.find(_initial_location);
        if(initial_location_it == locations.end())
            throw std::logic_error("Invalid initial location "+_initial_location+" for component");
        initial_location = _initial_location;
        current_location = initial_location;
    }

    auto get_enabled_edges(const symbol_table_t&) const -> std::vector<const edge_t*>;
    auto operator=(const std::string& new_location) -> component_t&;
};

using component_map_t = std::unordered_map<std::string, component_t>;

#endif //AALTITOAD_COMPONENT_H
