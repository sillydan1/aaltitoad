#ifndef AALTITOAD_LOCATION_H
#define AALTITOAD_LOCATION_H
#include <aaltitoadpch.h>

struct location_t {
    bool is_immediate;
    explicit location_t(bool is_immediate = false) : is_immediate{is_immediate} {}
};

using location_map_t = std::unordered_map<std::string, location_t>;

#endif
