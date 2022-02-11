#ifndef AALTITOAD_LOCATION_H
#define AALTITOAD_LOCATION_H
#include <aaltitoadpch.h>

struct location_t {
    std::string identifier;
    bool is_immediate;
    auto operator==(const location_t& o) const -> bool {
        return identifier == o.identifier && is_immediate == o.is_immediate;
    }
    auto operator!=(const location_t& o) const -> bool {
        return !this->operator==(o);
    }
};

#endif
