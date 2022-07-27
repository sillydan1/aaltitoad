#include <overload>
#include <nlohmann/json.hpp>
#include <extensions/exceptions/ntta_error.h>
#include "ntta.h"
#include "runtime/util/ntta_state_json.h"

#pragma clang diagnostic push
#pragma ide diagnostic ignored "readability-convert-member-functions-to-static"

void ntta_t::tick() {
    const auto* c_this = this;
    this->operator+=(c_this->tick());
}

state_diff_t ntta_t::tick() const {
    expr::symbol_table_t symbol_changes{};
    location_diff_t location_changes{};
    for(auto& component : state.components) {
        try {
            auto enabled_edges = component.second.get_enabled_edges(state.symbols);
            if (enabled_edges.empty())
                continue;
            if (enabled_edges.size() > 1)
                spdlog::warn("Non deterministic choice ({0} choices)", enabled_edges.size());

            // TODO: Pick strategy
            // TODO: Reject Overlapping Update Influences
            // TODO: Allow Idempotent updates
            auto &picked_edge = enabled_edges[0];
            symbol_changes += picked_edge->evaluate_updates(state.symbols);
            location_changes[component.first] = picked_edge->to;
        } catch (ntta_error& e) {
            throw ntta_error::in_component(component.first, e);
        }
    }
    return {location_changes, symbol_changes};
}

void ntta_t::tock() {
    this->operator+=(tock_values());
}

expr::symbol_table_t ntta_t::tock_values() const {
    expr::symbol_table_t tock_changes{};
    for(auto& tocker : tockers)
        tock_changes += tocker->tock(state.symbols);
    return tock_changes;
}

void ntta_t::operator+=(const state_diff_t& diff) {
    *this += diff.symbols;
    for(auto&& location_change : diff.locations) {
        auto component = state.components.find(location_change.first);
        if(component == state.components.end())
            throw std::logic_error("Unable to find component "+location_change.first);
        component->second = location_change.second;
    }
}

void ntta_t::operator+=(const expr::symbol_table_t& diff) {
    state.symbols += diff;
}

std::ostream& operator<<(std::ostream& os, const ntta_t& tta) {
    os << "State: {\nlocations\n[\n    ";
    for(auto& component : tta.state.components)
        os << component.second.current_location << ",";
    return os << "\n]\nsymbols\n<\n" << tta.state.symbols << ">}";
}

// Overload for << state_json << symbol_value_t
std::ostream& operator<<(json_ostream os, const expr::symbol_value_t& v) {
    std::visit(ya::overload{
            [&os](const bool& b) { os << std::boolalpha << b; },
            [&os](const std::string& s) { os << "\"" << s << "\""; },
            [&os](auto&& v) { os << v; }}, static_cast<const expr::underlying_symbol_value_t&>(v));
    return os.os;
}

std::ostream& operator<<(json_ostream jos, const ntta_t& tta) {
    std::stringstream ss{};
    ss << "{\"locations\":{";
    for(auto iter = tta.state.components.begin(); iter != tta.state.components.end(); ++iter) {
        if (iter != tta.state.components.begin())
            ss << ",";
        ss << "\"" << iter->first << "\":\"" << iter->second.current_location << "\"";
    }
    ss << "},";
    ss << "\"symbols\":{";
    for(auto iter = tta.state.symbols.begin(); iter != tta.state.symbols.end(); ++iter) {
        if(iter != tta.state.symbols.begin())
            ss << ",";
        ss << "\"" << iter->first << "\":" << stream_mods::json << iter->second << "";
    }
    ss << "}}";
    auto j = nlohmann::json::parse(ss.str());
    return jos << j;
}

#pragma clang diagnostic pop