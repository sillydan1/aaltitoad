#include "tta.h"
#include <aaltitoadpch.h>

namespace aaltitoad {
    auto ntta_t::collect_choices(const ya::combiner_iterator_list_t<choice_t>& iterator_list) -> std::optional<state_change_t> {
        state_change_t result{};
        for(auto& it : iterator_list) {
            if(result.symbol_changes.is_overlapping_and_not_idempotent(it->symbol_changes)) {
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
        ya::graph_builder<tta_t::graph_edge_iterator_t, std::string> edge_dependency_graph_builder{};
        // For each component
        //    For each enabled edge e
        //        add node 'e' to the graph
        //    connect all enabled edge nodes in this component
        // For each edge e1 added
        //     For each edge e2 added
        //          if e1.updates.is_overlapping_and_not_idempotent(e2.updates)
        //              connect e1 and e2 in the graph

        // x = "" // satisfiability query
        // m = "" // environment
        // For each node n in the graph
        //    if n.ingoing.empty() && n.outgoing.empty()
        //        x += " && " + n;
        //    m += n " := false;"
        // For each edge e in the graph
        //    x += " && (" + e.source + " xor " + e.target + ")"

        // solutions = []
        // solution = z3(m,x)
        // While(!solution.empty())
        //     solutions += solution
        //     x += " && !(" + solution + ")"
        //     solution = z3(m,x)

        // Convert the list of edge-solutions to state_change_t's
        // return that
        for(auto it = components.begin(); it != components.end(); ++it) {
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
