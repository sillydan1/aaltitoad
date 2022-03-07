#ifndef AALTITOAD_STATE_H
#define AALTITOAD_STATE_H
#include <aaltitoadpch.h>
#include <symbol_table.h>
#include "component.h"

using location_diff_t = std::unordered_map<std::string, std::string>;
struct state_diff_t {
    location_diff_t locations;
    symbol_table_t symbols;
};

struct state_t {
    component_map_t components{};
    symbol_table_t symbols{};
};

#endif //AALTITOAD_STATE_H
