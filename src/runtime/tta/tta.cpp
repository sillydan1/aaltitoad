#include "tta.h"
#include "drivers/z3_driver.h"
#include <algorithm>
#include <spdlog/spdlog.h>

namespace aaltitoad {
    auto ntta_t::state_change_t::operator+=(const choice_t& v) -> state_change_t & {
        location_changes.push_back(v.location_change);
        symbol_changes += v.symbol_changes;
        return *this;
    }

    auto ntta_t::eval_updates(expr::interpreter &i, const expr::compiler::compiled_expr_collection_t &t) -> expr::symbol_table_t {
        return expr::interpreter::evaluate(t,i,i,i);
    }

    auto ntta_t::eval_guard(expr::interpreter &i, const expr::compiler::compiled_expr_t &e) -> expr::symbol_value_t {
        return expr::interpreter::evaluate(e,i,i,i);
    }

    auto ntta_t::tick() -> std::vector<state_change_t> {
        auto problem = generate_enabled_choice_dependency_graph();
        auto solutions = tick_resolver{problem.dependency_graph}.solve();
        std::vector<state_change_t> result{};
        result.reserve(solutions.size());
        for(auto& solution : solutions) {
            state_change_t res{};
            for(auto& choice : solution)
                res += problem.choices.at(choice);
            result.push_back(res);
        }
        return result;
    }

    auto ntta_t::tock() const -> std::vector<expr::symbol_table_t> {
        // TODO: What to do if two tockers generate overlapping & non-idempotent symbol changes?
        std::vector<expr::symbol_table_t> result{};
        for(auto& tocker : tockers) {
            auto changes = tocker->tock(*this);
            result.insert(result.end(), changes.begin(), changes.end());
        }
        return result;
    }

    void ntta_t::add_tocker(std::unique_ptr<tocker_t> &&tocker) {
        tockers.push_back(std::move(tocker));
    }

    void ntta_t::apply(const state_change_t &changes)  {
        for(auto& location_change : changes.location_changes)
            location_change.component->second.current_location = location_change.new_location;
        apply_internal(changes.symbol_changes);
    }

    void ntta_t::apply(const expr::symbol_table_t &symbol_changes) {
        external_symbols += symbol_changes;
    }

    void ntta_t::apply_internal(const expr::symbol_table_t &symbol_changes) {
        symbols += symbol_changes;
    }

    auto ntta_t::generate_enabled_choice_dependency_graph() -> tick_resolver::choice_dependency_problem {
        // Build dependency graph and collect choices
        auto all_symbols = symbols + external_symbols;
        expr::interpreter i{all_symbols};
        tick_resolver::graph_type_builder edge_dependency_graph_builder{};
        std::unordered_map<std::string, choice_t> all_enabled_choices{};
        uint32_t uniqueness_counter = 0;
        for(auto component_it = components.begin(); component_it != components.end(); component_it++) {
            std::vector<std::string_view> enabled_edges{};
            for(auto edge : component_it->second.current_location->second.outgoing_edges) {
                if(!std::get<bool>(eval_guard(i,edge->second.data.guard)))
                    continue;
                edge_dependency_graph_builder.add_node({edge->first.identifier, edge});
                for(auto& choice : enabled_edges)
                    edge_dependency_graph_builder.add_edge(edge->first.identifier, std::string{choice}, uniqueness_counter++);
                enabled_edges.push_back(edge->first.identifier);
                all_enabled_choices[edge->first.identifier] = choice_t{edge,{component_it,edge->second.target},
                                                                       eval_updates(i,edge->second.data.updates)};
            }
        }
        // Add overlapping non-idempotent edges to the dependency graph
        for (auto it1 = all_enabled_choices.begin(); it1 != all_enabled_choices.end(); it1++) {
            for (auto it2 = it1; it2 != all_enabled_choices.end(); it2++) {
                if(it1 == it2)
                    continue;
                if (it1->second.symbol_changes.is_overlapping_and_not_idempotent(it2->second.symbol_changes))
                    // TODO: Warn about overlapping non-idempotent choices (unless silenced with -Wno-overlap-non-idem)
                    edge_dependency_graph_builder.add_edge(it1->first, it2->first, uniqueness_counter++);
            }
        }
        // TODO: Check that the builder is valid (when yalibs implements it)
        return {edge_dependency_graph_builder.optimize().build(), all_enabled_choices};
    }

    ntta_t::tick_resolver::tick_resolver(const graph_type& G) : G{G} {
        for(auto& n : G.nodes)
            N.insert(n.first);
    }

    auto ntta_t::tick_resolver::solve() -> solution_keys {
        solution_keys S{};
        for(auto& n : N)
            solve_recursive(S, {n});
        return S;
    }

    void ntta_t::tick_resolver::solve_recursive(solution_keys& S, const set& a) {
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

    auto ntta_t::tick_resolver::get_neighbors(const std::string& node_key) -> set {
        set value{};
        for(auto& e : G.nodes.at(node_key).outgoing_edges)
            value.insert(e->second.target->first);
        for(auto& e : G.nodes.at(node_key).ingoing_edges)
            value.insert(e->second.source->first);
        return value;
    }

    auto ntta_t::tick_resolver::get_postsets(const set& node_keys) -> set {
        set value{};
        for(auto& key : node_keys) {
            auto s = get_neighbors(key);
            std::set_union(value.begin(), value.end(), s.begin(), s.end(), std::inserter(value, value.begin()));
        }
        return value;
    }

    // TODO: Make a yalibs wrapper library for this
    auto ntta_t::tick_resolver::_union(const set& a, const set& b) -> set {
        set r{};
        std::set_union(a.begin(),a.end(),b.begin(),b.end(),std::inserter(r,r.begin()));
        return r;
    }

    auto ntta_t::tick_resolver::_difference(const set& a, const set& b) -> set {
        set r{};
        std::set_difference(a.begin(),a.end(),b.begin(),b.end(),std::inserter(r,r.begin()));
        return r;
    }
}

auto operator<<(std::ostream& os, const aaltitoad::ntta_t& state) -> std::ostream& {
    for (auto &component: state.components)
        os << component.first << ": " << component.second.current_location->first << "\n";
    return os << state.symbols;
}


