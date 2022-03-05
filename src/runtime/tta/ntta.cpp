#include "ntta.h"
#pragma clang diagnostic push
#pragma ide diagnostic ignored "readability-convert-member-functions-to-static"

void ntta_t::tick() {
    const auto* c_this = this;
    this->operator+=(c_this->tick());
}

state_diff_t ntta_t::tick() const {
    symbol_table_t symbol_changes{};
    location_diff_t location_changes{};
    for(auto&& component : state.components) {
        auto enabled_edges = component.second.get_enabled_edges(state.symbols);
        if(enabled_edges.empty())
            continue;
        if(enabled_edges.size() > 1)
            spdlog::warn("Non deterministic choice ({0} choices)", enabled_edges.size());
        symbol_changes += enabled_edges[0]->evaluate_updates(state.symbols);
        location_changes[component.first] = enabled_edges[0]->to.identifier;
    }
    return {location_changes, symbol_changes};
}

void ntta_t::tock() {
    const auto* c_this = this;
    this->operator+=(c_this->tock());
}

symbol_table_t ntta_t::tock() const {
    // TODO: Plugin-able tockers (blocking, buffered)
    //       interesting_tocker (blocking)
    //       piped_tocker (blocking)
    return {};
}

void ntta_t::operator+=(const state_diff_t& diff) {
    *this += diff.symbols;
    for(auto&& location_change : diff.locations)
        state.components[location_change.first] = location_change.second;
}

void ntta_t::operator+=(const symbol_table_t& diff) {
    state.symbols += diff;
}

#pragma clang diagnostic pop
