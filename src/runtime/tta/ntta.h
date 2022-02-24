#ifndef AALTITOAD_NTTA_H
#define AALTITOAD_NTTA_H
#include <aaltitoadpch.h>
#include "symbol_map.h"
#include "component.h"

using location_diff_t = std::unordered_map<std::string, std::string>;
struct state_diff_t {
    location_diff_t locations;
    symbol_map_t symbols;
};

struct state_t {
    std::unordered_map<std::string, component_t> components;
    symbol_map_t symbols;
};

/**
 * Networked Tick Tock Automata data structure (N)TTA
 * Consists of:
 *  - A set of parallel TTAs (in the form of components) and
 *  - A set of symbols shared across TTAs
 * */
struct ntta_t {
    void tick();
    [[nodiscard]] state_diff_t tick() const;
    void tock();
    [[nodiscard]] symbol_map_t tock() const;
    void operator+=(const state_diff_t& diff);
    void operator+=(const symbol_map_t& diff);

    state_t state;
};

namespace std {
    /** std::hash<ntta_t> implementation, so we can hash ntta states */
    template <>
    struct hash<ntta_t> {
        std::size_t operator()(const ntta_t& automata) const {
            std::size_t state_hash = 0;
            for(auto& component : automata.state.components)
                state_hash == 0 ?
                [&state_hash, &component](){ state_hash = std::hash<std::string>{}(component.second.current_location->first);}() :
                hash_combine(state_hash, component.second.current_location->first);
            hash_combine(state_hash, std::hash<symbol_map_t>{}(automata.state.symbols));
            return state_hash;
        }
    };
}

#endif
