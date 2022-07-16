#include "tta.h"
#include "drivers/z3_driver.h"

namespace aaltitoad {
    auto ntta_t::tick() -> std::vector<state_change_t> {
        expr::interpreter i{symbols};
        auto eval_updates = [&i](const expr::compiler::compiled_expr_collection_t& t){return expr::interpreter::evaluate(t,i,i,i);};
        auto eval_guard = [&i](const expr::compiler::compiled_expr_t& t){return expr::interpreter::evaluate(t,i,i,i);};

        // For each component
        //    For each enabled edge e
        //        add node 'e' to the graph
        //    connect all enabled edge nodes in this component
        ya::graph_builder<tta_t::graph_edge_iterator_t, std::string> edge_dependency_graph_builder{};
        std::vector<choice_t> all_enabled_choices{};
        for(auto component_it = components.begin(); component_it != components.end(); component_it++) {
            std::vector<tta_t::graph_edge_iterator_t> enabled_edges{};
            for(auto& edge : component_it->second.current_location->second.outgoing_edges) {
                if(!std::get<bool>(eval_guard(edge->second.data.guard)))
                    continue;
                enabled_edges.push_back(edge);
            }
            for(auto it1 = enabled_edges.begin(); it1 != enabled_edges.end(); it1++) {
                edge_dependency_graph_builder.add_node({*it1});
                auto& e = *it1;
                all_enabled_choices.emplace_back(choice_t{e, {component_it, e->second.target}, eval_updates(e->second.data.updates)});
                for(auto it2 = it1; it2 != enabled_edges.end(); it2++)
                    edge_dependency_graph_builder.add_edge(*it1, *it2, component_it->first);
            }
        }
        // For each edge e1 added
        //     For each edge e2 added
        //          if e1.updates.is_overlapping_and_not_idempotent(e2.updates)
        //              connect e1 and e2 in the graph
        for(auto it1 = all_enabled_choices.begin(); it1 != all_enabled_choices.end(); it1++) {
            for(auto it2 = it1; it2 != all_enabled_choices.end(); it2++) {
                if(it1->symbol_changes.is_overlapping_and_not_idempotent(it2->symbol_changes))
                    edge_dependency_graph_builder.add_edge(it1->edge, it2->edge, "");
            }
        }

        // For each node n in the graph
        //    if n.ingoing.empty() && n.outgoing.empty()
        //        x += " && " + n;
        //    m += n " := false;"
        // For each edge e in the graph
        //    x += " && (" + e.source + " xor " + e.target + ")"
        std::stringstream satisfiability_query{}; // TODO: Implement your own query stringstream with seperated &&'s
        expr::symbol_table_t environment{};
        auto graph = edge_dependency_graph_builder.build();
        for(auto& n : graph.nodes) {
            if(n.second.outgoing_edges.empty() && n.second.ingoing_edges.empty())
                satisfiability_query << n.second.data->second.data.identifier; // TODO: uuid's are not valid identifiers
            environment[n.second.data->second.data.identifier] = false; // TODO: uuid's are not valid identifiers
        }
        for(auto& e : graph.edges)
            satisfiability_query << "(" << e.second.source->second.data->second.data.identifier //TODO: uuid's are not valid identifiers
                                        << " ^^ "
                                        << e.second.target->second.data->second.data.identifier //TODO: uuid's are not valid identifiers
                                        << ")";

        // solution = z3(m,x)
        // While(!solution.empty())
        //     solutions += solution
        //     x += " && !(" + solution + ")"
        //     solution = z3(m,x)
        std::vector<expr::symbol_table_t> solutions{};
        expr::z3_driver sat_solver{environment};
        if(sat_solver.parse(satisfiability_query.str()) != 0)
            throw std::logic_error(sat_solver.error);
        auto solution = sat_solver.result;
        while(!solution.empty()) {
            solutions.push_back(solution);
            satisfiability_query << "!(" << solution << ")"; // TODO: implement << overload for expr::symbol_table_t's
            if(sat_solver.parse(satisfiability_query.str()) != 0)
                throw std::logic_error(sat_solver.error);
            solution = sat_solver.result;
        }

        // Convert the list of edge-solutions to state_change_t's
        // return that
        return {};
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
