#include "tta.h"
#include <permutation>

namespace aaltitoad {
    auto ntta_t::tick() -> std::vector<state_change_t> {
        std::vector<state_change_t> choices{};
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
                component_choices.emplace_back(choice_t{{it, edge->second.target}, eval_updates(edge->second.data.updates)});
            }
            if(!component_choices.empty())
                available_choices.push_back(component_choices);
        }
        // TODO: calculate all permutations of available_choices

        auto changes = ya::generate_permutations(available_choices, );
        return choices;
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
