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
 *
 *  todolist:
 *  TODO: Implement std::hash<ntta_t>
 * */
struct ntta_t {
    explicit ntta_t(state_t&& initial_state) : state{initial_state}, tockers{} {}

    void tick();
    [[nodiscard]] state_diff_t tick() const;
    void tock();
    [[nodiscard]] expr::symbol_table_t tock_values() const;
    void operator+=(const state_diff_t& diff);
    void operator+=(const expr::symbol_table_t& diff);

    state_t state;
    std::vector<std::unique_ptr<tocker_t>> tockers;
};

std::ostream& operator<<(std::ostream& os, const ntta_t& tta);
std::ostream& operator<<(json_ostream jos, const ntta_t& tta);

#endif
