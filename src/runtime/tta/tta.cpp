#include "tta.h"
#include <aaltitoadpch.h>

namespace aaltitoad {
    auto ntta_t::collect_choices(const ya::combiner_iterator_list_t<choice_t>& iterator_list) -> std::optional<state_change_t> {
        state_change_t result{};
        for(auto& it : iterator_list) {
            if(result.symbol_changes.is_overlapping(it->symbol_changes)) {
                spdlog::debug("overlapping updates in edge '{0}'", it->edge_identifier);
                if(spdlog::should_log(spdlog::level::debug)) {
                    std::stringstream ss{};
                    for(auto& debug_it : iterator_list)
                        ss << debug_it->edge_identifier << " ";
                    spdlog::debug("all overlapping edges: {0}", ss.str());
                }
                return {};
            }
            result.symbol_changes += it->symbol_changes;
            result.location_changes.push_back(it->location_change);
        }
        return result;
    }

    auto ntta_t::tick() -> std::vector<state_change_t> {
        expr::interpreter i{symbols};
        auto eval_updates = [&i](const expr::compiler::compiled_expr_collection_t& t){return expr::interpreter::evaluate(t,i,i,i);};
        auto eval_guard = [&i](const expr::compiler::compiled_expr_t& t){return expr::interpreter::evaluate(t,i,i,i);};

        std::vector<std::vector<choice_t>> available_choices{};
        for(auto it = components.begin(); it != components.end(); ++it) {
            // TODO: This could be thread-pooled
            std::vector<choice_t> component_choices{};
            for(auto& edge : it->second.current_location->second.outgoing_edges) {
                if(!std::get<bool>(eval_guard(edge->second.data.guard)))
                    continue;
                component_choices.emplace_back(choice_t{edge->second.data.identifier, {it, edge->second.target}, eval_updates(edge->second.data.updates)});
            }
            if(!component_choices.empty())
                available_choices.push_back(component_choices);
        }
        return ya::generate_permutations(available_choices, collect_choices);
    }

    auto ntta_t::tock() const -> expr::symbol_table_t {
        return {}; // TODO: Implement injectable tockers
    }

    void ntta_t::apply(const state_change_t &changes)  {
        for(auto& location_change : changes.location_changes)
            location_change.component->second.current_location = location_change.new_location;
        symbols += changes.symbol_changes;
    }

    void ntta_t::apply(const expr::symbol_table_t &symbol_changes) {
        symbols += symbol_changes;
    }
}

auto operator<<(std::ostream& os, const aaltitoad::ntta_t& state) -> std::ostream& {
    for (auto &component: state.components)
        os << component.first << ": " << component.second.current_location->first << "\n";
    return os << state.symbols;
}
