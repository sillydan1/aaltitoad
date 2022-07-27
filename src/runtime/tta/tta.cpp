#include "tta.h"
#include "drivers/z3_driver.h"
#include <aaltitoadpch.h>
#include <set>
#include <algorithm>

namespace aaltitoad {
    auto ntta_t::state_change_t::operator+=(const choice_t& v) -> state_change_t & {
        location_changes.push_back(v.location_change);
        symbol_changes += v.symbol_changes;
        return *this;
    }

    struct tick_resolver {
        using set = std::set<std::string>;
        using solution_keys = std::vector<std::set<std::string>>;
        using graph_type = ya::graph<tta_t::graph_edge_iterator_t, std::string, std::string>;

        explicit tick_resolver(const graph_type& G) : G{G} {
            for(auto& n : G.nodes)
                N.insert(n.first);
        }

        solution_keys solve() {
            solution_keys S{};
            for(auto& n : N)
                subsolve(S, {n});
            return S;
        }
    private:
        void subsolve(solution_keys& S, const set& a) {
            for(auto& s : S) {
                if(std::includes(s.begin(),s.end(),a.begin(),a.end()))
                    return;
            }
            std::set<std::string> e{};
            auto p = get_postsets(a);
            std::set_union(a.begin(), a.end(), p.begin(), p.end(), std::inserter(e, e.begin()));
            std::set<std::string> k{};
            std::set_difference(N.begin(), N.end(), e.begin(), e.end(), std::inserter(k, k.begin()));
            if(k.empty())
                S.push_back(a);
            for(auto& m : k) {
                std::set<std::string> ms = {m};
                std::set<std::string> r{};
                std::set_union(a.begin(), a.end(), ms.begin(), ms.end(), std::inserter(r,r.begin()));
                subsolve(S,r);
            }
        }

        set get_postset(const std::string& node_key) {
            set value{};
            for(auto& e : G.nodes.at(node_key).outgoing_edges)
                value.insert(e->second.target->first);
            return value;
        }

        set get_postsets(const set& node_keys) {
            set value{};
            for(auto& key : node_keys) {
                auto s = get_postset(key);
                std::set_union(value.begin(), value.end(), s.begin(), s.end(), std::inserter(value, value.begin()));
            }
            return value;
        }

        const graph_type& G;
        set N;
    };

    auto ntta_t::tick() -> std::vector<state_change_t> {
        expr::interpreter i{symbols};
        auto eval_updates = [&i](const expr::compiler::compiled_expr_collection_t& t){return expr::interpreter::evaluate(t,i,i,i);};
        auto eval_guard = [&i](const expr::compiler::compiled_expr_t& t){return expr::interpreter::evaluate(t,i,i,i);};

        ya::graph_builder<tta_t::graph_edge_iterator_t, std::string, std::string> edge_dependency_graph_builder{};
        std::unordered_map<std::string, choice_t> all_enabled_choices{};
        for(auto component_it = components.begin(); component_it != components.end(); component_it++) {
            std::vector<tta_t::graph_edge_iterator_t> enabled_edges{};
            for(auto edge : component_it->second.current_location->second.outgoing_edges) {
                if(!std::get<bool>(eval_guard(edge->second.data.guard)))
                    continue;
                enabled_edges.push_back(edge);
                edge_dependency_graph_builder.add_node({edge->first.identifier, edge});
                auto x = edge->second.target;
                all_enabled_choices[edge->first.identifier] = choice_t{edge, {component_it, x}, eval_updates(edge->second.data.updates)};
            }
            for(auto it1 = enabled_edges.begin(); it1 != enabled_edges.end()-1; it1++) {
                const auto& itt = it1; // Force iterator copying
                for(auto it2 = itt+1; it2 != enabled_edges.end(); it2++) {
                    edge_dependency_graph_builder.add_edge((*itt)->first.identifier,
                                                           (*it2)->first.identifier,
                                                           ya::uuid_v4());
                    edge_dependency_graph_builder.add_edge((*it2)->first.identifier,
                                                           (*itt)->first.identifier,
                                                           ya::uuid_v4());
                }
            }
        }

        for (auto it1 = all_enabled_choices.begin(); it1 != all_enabled_choices.end(); it1++) {
            for (auto it2 = it1; it2 != all_enabled_choices.end(); it2++) {
                if(it1 == it2)
                    continue;
                if (it1->second.symbol_changes.is_overlapping_and_not_idempotent(it2->second.symbol_changes)) {
                    spdlog::trace("{0} and {1} are overlapping and not idempotent",
                                  it1->second.edge->second.data.identifier,
                                  it2->second.edge->second.data.identifier);
                    edge_dependency_graph_builder.add_edge(it1->second.edge->first.identifier,
                                                           it2->second.edge->first.identifier,
                                                           ya::uuid_v4());
                    edge_dependency_graph_builder.add_edge(it2->second.edge->first.identifier,
                                                           it1->second.edge->first.identifier,
                                                           ya::uuid_v4());
                }
            }
        }

        // TODO: Make a "build_doubly_linked" or something to avoid the extra construction code above
        auto solutions = tick_resolver{edge_dependency_graph_builder.build()}.solve();

        spdlog::debug("{0} possible solutions to this tick", solutions.size());
        std::vector<state_change_t> result{}; result.reserve(solutions.size());
        for(auto& solution : solutions) {
            std::stringstream debug_stream{};
            debug_stream << "solution: ";
            state_change_t res{};
            for(auto& choice : solution) {
                debug_stream << choice << " ";
                res += all_enabled_choices.at(choice);
            }
            result.push_back(res);
            spdlog::debug(debug_stream.str());
        }
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
