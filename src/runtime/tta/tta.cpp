#include "tta.h"
#include "drivers/z3_driver.h"
#include <set>
#include <algorithm>
#include <spdlog/spdlog.h>

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
                solve_recursive(S, {n});
            return S;
        }
    private:
        void solve_recursive(solution_keys& S, const set& a) {
            for(auto& s : S) {
                if(std::includes(s.begin(),s.end(),a.begin(),a.end()))
                    return;
            }
            auto k = _difference(N, _union(a, get_postsets(a)));
            if(k.empty())
                S.push_back(a);
            for(auto& m : k)
                solve_recursive(S,_union(a,{m}));
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

        static auto _union(const set& a, const set& b)-> set {
            set r{};
            std::set_union(a.begin(),a.end(),b.begin(),b.end(),std::inserter(r,r.begin()));
            return r;
        }

        static auto _difference(const set& a, const set& b) -> set {
            set r{};
            std::set_difference(a.begin(),a.end(),b.begin(),b.end(),std::inserter(r,r.begin()));
            return r;
        }

        const graph_type& G;
        set N;
    };

    auto ntta_t::eval_updates(expr::interpreter &i, const expr::compiler::compiled_expr_collection_t &t) -> expr::symbol_table_t {
        return expr::interpreter::evaluate(t,i,i,i);
    }

    auto ntta_t::eval_guard(expr::interpreter &i, const expr::compiler::compiled_expr_t &e) -> expr::symbol_value_t {
        return expr::interpreter::evaluate(e,i,i,i);
    }

    auto ntta_t::tick() -> std::vector<state_change_t> {
        expr::interpreter i{symbols};

        // Build dependency graph and collect choices
        ya::graph_builder<tta_t::graph_edge_iterator_t, std::string, std::string> edge_dependency_graph_builder{};
        std::unordered_map<std::string, choice_t> all_enabled_choices{};
        for(auto component_it = components.begin(); component_it != components.end(); component_it++) {
            std::vector<std::string_view> enabled_edges{};
            for(auto edge : component_it->second.current_location->second.outgoing_edges) {
                if(!std::get<bool>(eval_guard(i,edge->second.data.guard)))
                    continue;
                edge_dependency_graph_builder.add_node({edge->first.identifier, edge});
                for(auto& choice : enabled_edges) {
                    edge_dependency_graph_builder.add_edge(edge->first.identifier,std::string{choice},ya::uuid_v4());
                    edge_dependency_graph_builder.add_edge(std::string{choice},edge->first.identifier,ya::uuid_v4());
                }
                enabled_edges.push_back(edge->first.identifier);
                all_enabled_choices[edge->first.identifier] = choice_t{edge,
                                 {component_it, edge->second.target},
                                 eval_updates(i,edge->second.data.updates)};
            }
        }
        // Add overlapping non-idempotent edges to the dependency graph
        for (auto it1 = all_enabled_choices.begin(); it1 != all_enabled_choices.end(); it1++) {
            for (auto it2 = it1; it2 != all_enabled_choices.end(); it2++) {
                if(it1 == it2)
                    continue;
                if (it1->second.symbol_changes.is_overlapping_and_not_idempotent(it2->second.symbol_changes)) {
                    edge_dependency_graph_builder.add_edge(it1->first,it2->first,ya::uuid_v4());
                    edge_dependency_graph_builder.add_edge(it2->first,it1->first,ya::uuid_v4());
                }
            }
        }

        // TODO: Make a "build_doubly_linked" or something to avoid the extra construction code above
        auto solutions = tick_resolver{edge_dependency_graph_builder.build()}.solve();

        std::vector<state_change_t> result{};
        result.reserve(solutions.size());
        for(auto& solution : solutions) {
            state_change_t res{};
            for(auto& choice : solution)
                res += all_enabled_choices.at(choice);
            result.push_back(res);
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
