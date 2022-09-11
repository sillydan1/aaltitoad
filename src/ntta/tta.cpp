#include "tta.h"
#include "drivers/z3_driver.h"
#include <setwrappers>
#include <algorithm>
#include <spdlog/spdlog.h>
#include <warnings.h>

namespace aaltitoad {
    auto ntta_t::state_change_t::operator+=(const choice_t& v) -> state_change_t & {
        location_changes.push_back(v.location_change);
        symbol_changes += v.symbol_changes;
        return *this;
    }

    auto ntta_t::eval_updates(expr::interpreter &i, const expr::compiler::compiled_expr_collection_t &t) -> expr::symbol_table_t {
        return i.evaluate(t);
    }

    auto ntta_t::eval_guard(expr::interpreter &i, const expr::compiler::compiled_expr_t &e) -> expr::symbol_value_t {
        return i.evaluate(e);
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
        std::vector<expr::symbol_table_t> result{};
        for(auto& tocker : tockers) {
            auto changes = tocker->tock(*this);
            result.insert(result.end(), changes.begin(), changes.end());
        }
        return result;
    }

    auto ntta_t::add_tocker(const std::shared_ptr<tocker_t>& tocker) -> ntta_t& {
        tockers.push_back(tocker);
        return *this;
    }

    void ntta_t::apply(const state_change_t &changes)  {
        for(auto& location_change : changes.location_changes)
            components.at(location_change.component->first).current_location = location_change.new_location; // TODO: Maybe state-changes should just keep component name instead of a reference?
        apply_internal(changes.symbol_changes);
    }

    void ntta_t::apply(const expr::symbol_table_t &symbol_changes) {
        external_symbols.overwrite_elements(symbol_changes);
    }

    void ntta_t::apply(const std::vector<expr::symbol_table_t>& symbol_change_list) {
        expr::symbol_table_t combined_changes{};
        for(auto& changes : symbol_change_list) {
            if(combined_changes.is_overlapping_and_not_idempotent(changes))
                // TODO: Also provide the data if high verbosity
                warnings::warn(overlap_idem, "overlapping and non-idempotent changes in tocker-change application, will overwrite depending on the order");
            combined_changes += changes;
        }
        apply(combined_changes);
    }

    void ntta_t::apply_internal(const expr::symbol_table_t &symbol_changes) {
        symbols += symbol_changes;
    }

    auto ntta_t::generate_enabled_choice_dependency_graph() -> tick_resolver::choice_dependency_problem {
        // TODO: Implement the optimized version of this (see notes)
        // Build dependency graph and collect choices
        auto all_symbols = symbols + external_symbols;
        expr::interpreter i{all_symbols};
        tick_resolver::graph_type_builder edge_dependency_graph_builder{};
        std::unordered_map<std::string, choice_t> all_enabled_choices{};
        uint32_t uniqueness_counter = 0;
        for(auto component_it = components.begin(); component_it != components.end(); component_it++) {
            std::vector<std::string_view> enabled_edges{};
            for(auto edge : component_it->second.current_location->second.outgoing_edges) {
                if(!std::get<bool>(i.evaluate(edge->second.data.guard)))
                    continue;
                edge_dependency_graph_builder.add_node({edge->first.identifier, edge});
                for(auto& choice : enabled_edges)
                    edge_dependency_graph_builder.add_edge(edge->first.identifier, std::string{choice}, uniqueness_counter++);
                enabled_edges.push_back(edge->first.identifier);
                all_enabled_choices[edge->first.identifier] = choice_t{edge,{component_it,edge->second.target},
                                                                       i.evaluate(edge->second.data.updates)};
            }
        }
        // Add overlapping non-idempotent edges to the dependency graph
        for (auto it1 = all_enabled_choices.begin(); it1 != all_enabled_choices.end(); it1++) {
            for (auto it2 = it1; it2 != all_enabled_choices.end(); it2++) {
                if(it1 == it2)
                    continue;
                if (it1->second.symbol_changes.is_overlapping_and_not_idempotent(it2->second.symbol_changes)) {
                    // TODO: Also provide the data if high verbosity
                    warnings::warn(overlap_idem, "overlapping and non-idempotent changes in tick-change calculation");
                    edge_dependency_graph_builder.add_edge(it1->first, it2->first, uniqueness_counter++);
                }
            }
        }
        try {
            return { edge_dependency_graph_builder.validate().optimize().build(), all_enabled_choices };
        } catch(std::exception& e) {
            spdlog::critical("unable to generate enabled choice dependency graph: '{0}' please report this as an issue on github.com/sillydan1/AALTITOAD", e.what());
            throw e;
        }
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
        auto k = ya::set_difference(N, ya::set_union(a, get_all_neighbors(a)));
        if(k.empty())
            S.push_back(a);
        for(auto& m : k)
            solve_recursive(S,ya::set_union(a,{m}));
    }

    auto ntta_t::tick_resolver::get_neighbors(const std::string& node_key) -> set {
        set value{};
        for(auto& e : G.nodes.at(node_key).outgoing_edges)
            value.insert(e->second.target->first);
        for(auto& e : G.nodes.at(node_key).ingoing_edges)
            value.insert(e->second.source->first);
        return value;
    }

    auto ntta_t::tick_resolver::get_all_neighbors(const set& node_keys) -> set {
        set value{};
        for(auto& key : node_keys) {
            auto s = get_neighbors(key);
            std::set_union(value.begin(), value.end(), s.begin(), s.end(), std::inserter(value, value.begin()));
        }
        return value;
    }

    auto ntta_t::to_string() const -> std::string {
        std::stringstream ss{};
        ss << *this;
        return ss.str();
    }
}

auto operator<<(std::ostream& os, const aaltitoad::ntta_t& state) -> std::ostream& {
    for (auto &component: state.components)
        os << component.first << ": " << component.second.current_location->first << "\n";
    return os << state.symbols << state.external_symbols;
}

auto operator+(const aaltitoad::ntta_t& state, const aaltitoad::ntta_t::state_change_t& change) -> aaltitoad::ntta_t {
    auto cpy = state;
    cpy.apply(change);
    return cpy;
}

auto operator+(const aaltitoad::ntta_t& state, const expr::symbol_table_t& external_symbol_changes) -> aaltitoad::ntta_t {
    auto cpy = state;
    cpy.apply(external_symbol_changes);
    return cpy;
}

auto operator==(const aaltitoad::ntta_t& a, const aaltitoad::ntta_t& b) -> bool {
    // compare locations
    for(auto& c : a.components)
        if(c.second.current_location != b.components.at(c.first).current_location)
            return false;
    // compare symbol tables
    return a.symbols == b.symbols && a.external_symbols == b.external_symbols;
}
