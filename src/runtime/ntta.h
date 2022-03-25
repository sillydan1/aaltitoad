#ifndef AALTITOAD_NTTA_H
#define AALTITOAD_NTTA_H
#include <aaltitoadpch.h>
#include "state.h"
#include "extensions/hash_combine"
#include "runtime/util/ntta_state_json.h"
#include "tocker.h"

/**
 * Networked Tick Tock Automata data structure (N)TTA
 * Consists of:
 *  - A set of parallel TTAs (in the form of components) and
 *  - A set of symbols shared across TTAs
 * */
struct ntta_t {
    explicit ntta_t(state_t&& initial_state) : state{initial_state}, tockers{} {}

    void tick();
    [[nodiscard]] state_diff_t tick() const;
    void tock();
    [[nodiscard]] symbol_table_t tock_values();
    void operator+=(const state_diff_t& diff);
    void operator+=(const symbol_table_t& diff);

    state_t state;
    std::vector<std::unique_ptr<tocker_t>> tockers;
};

std::ostream& operator<<(std::ostream& os, const ntta_t& tta);
std::ostream& operator<<(json_ostream jos, const ntta_t& tta);

namespace std {
    /** std::hash<ntta_t> implementation, so we can hash ntta states */
    template <>
    struct hash<ntta_t> {
        std::size_t operator()(const ntta_t& automata) const {
            std::size_t state_hash = 0;
            for(auto& component : automata.state.components)
                state_hash == 0 ?
                [&state_hash, &component](){ state_hash = std::hash<std::string>{}(component.second.current_location);}() :
                hash_combine(state_hash, component.second.current_location);
            // TODO: Implement std::hash<symbol_table_t>
            // hash_combine(state_hash, std::hash<symbol_table_t>{}(automata.state.symbols));
            return state_hash;
        }
    };
}

#endif
