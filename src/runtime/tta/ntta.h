#ifndef AALTITOAD_NTTA_H
#define AALTITOAD_NTTA_H
#include <aaltitoadpch.h>
#include "symbol_map.h"
#include "component.h"

/**
 * Networked Tick Tock Automata data structure (N)TTA
 * Consists of:
 *  - A set of parallel TTAs (in the form of components) and
 *  - A set of symbols shared across TTAs
 * */
struct ntta_t {
    struct diff_t {
        using ntta_location_diff_t = std::vector<std::vector<location_t>::const_iterator>;
        ntta_location_diff_t locations;
        symbol_map_t symbols;
    };

    void tick();
    [[nodiscard]] diff_t tick() const;
    void tock();
    [[nodiscard]] symbol_map_t tock() const;
    void operator+=(const diff_t& diff);
    void operator+=(const symbol_map_t& diff);

    std::vector<component_t> components;
    symbol_map_t symbols;
};

namespace std {
    /** std::hash<ntta_t> implementation, so we can hash ntta states */
    template <>
    struct hash<ntta_t> {
        std::size_t operator()(const ntta_t& state) const {
            std::size_t state_hash = 0;
            for(auto& component : state.components)
                state_hash == 0 ?
                [&state_hash, &component](){ state_hash = std::hash<std::string>{}(component.current_location->identifier);}() :
                hash_combine(state_hash, component.current_location->identifier);
            hash_combine(state_hash, std::hash<symbol_map_t>{}(state.symbols));
            return state_hash;
        }
    };
}

#endif
