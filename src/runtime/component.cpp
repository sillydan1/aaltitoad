#include <extensions/string_extensions.h>
#include "component.h"

auto component_t::get_enabled_edges(const symbol_table_t& environment) const -> std::vector<const edge_t*> {
    std::vector<const edge_t*> enabled_edges{};
    for(auto&& edge : edges) {
        if(edge.from != current_location)
            continue;
        if(edge.is_satisfied(environment))
            enabled_edges.push_back(&edge);
    }
    return enabled_edges;
}

auto component_t::operator=(const std::string& new_location) -> component_t & {
    const auto& it = locations.find(new_location);
    if(it == locations.end())
        throw std::logic_error((std::string)(string_builder{} << "Location '" << new_location << "' is not a location in this component"));
    current_location = it->first;
    return *this;
}
