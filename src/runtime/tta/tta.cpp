#include "tta.h"
#include "drivers/z3_driver.h"
#include <aaltitoadpch.h>

namespace aaltitoad {
    struct query_stream {
        std::stringstream ss = {};
        int sub_expression_count = 0;
        auto str() const -> std::string {
            return ss.str();
        }
    };
    struct xor_q {
        std::string left, right;
    };
    auto operator<<(query_stream& s, const std::string& expression) -> query_stream& {
        if(s.sub_expression_count++ > 0)
            s.ss << " && ";
        s.ss << "(" << expression << ")";
        return s;
    }
    auto operator<<(query_stream& s, const xor_q& sub_expr) -> query_stream& {
        if(s.sub_expression_count++)
            s.ss << " && ";
        s.ss << "(" << sub_expr.left << " => !" << sub_expr.right << " && "
                    << sub_expr.right << " => !" << sub_expr.left << ")";
        return s;
    }
    auto operator<<(query_stream& s, const expr::symbol_table_t& solution) -> query_stream& {
        if(s.sub_expression_count++)
            s.ss << " && ";
        s.ss << "!(";
        std::string sep{};
        for(auto& el : solution) {
            s.ss << sep;
            if(!std::get<bool>(el.second))
                s.ss << "!";
            s.ss << el.first;
            sep = " && ";
        }
        s.ss << ")";
        return s;
    }

    auto ntta_t::state_change_t::operator+=(const choice_t& v) -> state_change_t & {
        location_changes.push_back(v.location_change);
        symbol_changes += v.symbol_changes;
        return *this;
    }

    auto ntta_t::tick() -> std::vector<state_change_t> {
        expr::interpreter i{symbols};
        auto eval_updates = [&i](const expr::compiler::compiled_expr_collection_t& t){return expr::interpreter::evaluate(t,i,i,i);};
        auto eval_guard = [&i](const expr::compiler::compiled_expr_t& t){return expr::interpreter::evaluate(t,i,i,i);};

        // For each component
        //    For each enabled edge e
        //        add node 'e' to the sat_graph
        //    connect all enabled edge nodes in this component
        ya::graph_builder<tta_t::graph_edge_iterator_t, std::string> edge_dependency_graph_builder{};
        std::unordered_map<std::string, choice_t> all_enabled_choices{};
        query_stream satisfiability_query{};
        for(auto component_it = components.begin(); component_it != components.end(); component_it++) {
            std::vector<tta_t::graph_edge_iterator_t> enabled_edges{};
            std::stringstream ss{};
            std::string sep{};
            for(auto edge : component_it->second.current_location->second.outgoing_edges) {
                if(!std::get<bool>(eval_guard(edge->second.data.guard)))
                    continue;
                enabled_edges.push_back(edge);
                edge_dependency_graph_builder.add_node({edge});
                auto x = edge->second.target;
                all_enabled_choices[edge->first.identifier] = choice_t{edge, {component_it, x}, eval_updates(edge->second.data.updates)};
                ss << sep << edge->first.identifier;
                sep = "||";
            }
            for(auto it1 = enabled_edges.begin(); it1 != enabled_edges.end()-1; it1++) {
                const auto& itt = it1; // Force iterator copying
                for(auto it2 = itt+1; it2 != enabled_edges.end(); it2++) {
                    edge_dependency_graph_builder.add_edge(*itt, *it2, component_it->first);
                    satisfiability_query << xor_q{(*it1)->second.data.identifier,
                                                (*it2)->second.data.identifier};
                }
            }
            if(!ss.str().empty() && enabled_edges.size() > 1)
                satisfiability_query << ss.str();
        }

        // For each edge e1 added
        //     For each edge e2 added
        //          if e1.updates.is_overlapping_and_not_idempotent(e2.updates)
        //              connect e1 and e2 in the sat_graph
        for (auto it1 = all_enabled_choices.begin(); it1 != all_enabled_choices.end(); it1++) {
            for (auto it2 = it1; it2 != all_enabled_choices.end(); it2++) {
                if(it1 == it2)
                    continue;
                if (it1->second.symbol_changes.is_overlapping_and_not_idempotent(it2->second.symbol_changes)) {
                    spdlog::trace("{0} and {1} are overlapping and not idempotent",
                                  it1->second.edge->second.data.identifier,
                                  it2->second.edge->second.data.identifier);
                    edge_dependency_graph_builder.add_edge(it1->second.edge, it2->second.edge, ya::uuid_v4());
                    satisfiability_query << xor_q{it1->second.edge->second.data.identifier,
                                                  it2->second.edge->second.data.identifier};
                }
            }
        }

        // For each node n in the sat_graph
        //    if n.ingoing.empty() && n.outgoing.empty()
        //        x += " && " + n;
        //    m += n " := false;"
        // For each edge e in the sat_graph
        //    x += " && (" + e.source + " xor " + e.target + ")"
        expr::symbol_table_t environment{};
        auto sat_graph = edge_dependency_graph_builder.build();
        for(auto& n : sat_graph.nodes) {
            if(n.second.outgoing_edges.empty() && n.second.ingoing_edges.empty())
                satisfiability_query << n.second.data->second.data.identifier;
            environment[n.second.data->second.data.identifier] = false;
        }

        // solution = z3(m,x)
        // While(!solution.empty())
        //     solutions += solution
        //     x += " && !(" + solution + ")"
        //     solution = z3(m,x)
        std::vector<expr::symbol_table_t> solutions{};
        expr::z3_driver sat_solver{environment};
        spdlog::debug("SAT query: {0}", satisfiability_query.str());
        if(sat_solver.parse(satisfiability_query.str()) != 0)
            throw std::logic_error(sat_solver.error);
        auto solution = sat_solver.result;
        while(!solution.empty()) {
            solutions.push_back(solution);
            sat_solver.result = {};
            satisfiability_query << solution;
            if(sat_solver.parse(satisfiability_query.str()) != 0)
                throw std::logic_error(sat_solver.error);
            solution = sat_solver.result;
        }

        // Convert the list of edge-solutions to state_change_t's
        std::vector<state_change_t> result{}; result.reserve(solutions.size());
        for(auto& sol : solutions) {
            state_change_t choice{};
            for(auto& el : sol) {
                if(std::get<bool>(el.second))
                    choice += all_enabled_choices.at(el.first);
            }
            result.push_back(choice);
        }
        spdlog::debug("{0} possible solutions to this tick", result.size());
        return result;
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
