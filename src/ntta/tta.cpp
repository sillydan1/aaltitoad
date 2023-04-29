/**
 * aaltitoad - a verification engine for tick tock automata models
   Copyright (C) 2023 Asger Gitz-Johansen

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include "tta.h"
#include "symbol_table.h"
#include <setwrappers>
#include <algorithm>
#include <spdlog/spdlog.h>
#include <util/warnings.h>

namespace aaltitoad {
    auto ntta_t::state_change_t::operator+=(const choice_t& v) -> state_change_t & {
        location_changes.push_back(v.location_change);
        symbol_changes += v.symbol_changes;
        return *this;
    }

    auto ntta_t::eval_updates(expression_driver& i, const expr::syntax_tree_collection_t& t) -> expr::symbol_table_t {
        return i.evaluate(t);
    }

    auto ntta_t::eval_guard(expression_driver& i, const expr::syntax_tree_t& e) -> expr::symbol_value_t {
        return i.evaluate(e);
    }

    auto ntta_t::tick() -> std::vector<state_change_t> {
        auto problem = calculate_edge_dependency_graph();
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
            components.at(location_change.component->first).current_location = location_change.new_location;
        apply(changes.symbol_changes);
    }

    void ntta_t::apply(const expr::symbol_table_t& symbol_changes) {
        // apply changes to internal and external symbols (overwrite, dont insert)
        symbols *= symbol_changes;
        external_symbols *= symbol_changes;
    }

    auto conflict_string(const expr::symbol_table_t& a, const expr::symbol_table_t& b) -> std::vector<std::string> {
        std::vector<std::string> result{};
        for(auto& v : a) {
            if(b.contains(v.first) && std::get<bool>(b.get(v.first) != v.second)) {
                std::stringstream ss{}; ss << "\t- conflict: " << v.first << " (" << b.get(v.first) << " / " << v.second << ")";
                result.push_back(ss.str());
            }
        }
        return result;
    }

    void ntta_t::apply(const std::vector<expr::symbol_table_t>& symbol_change_list) {
        expr::symbol_table_t combined_changes{};
        for(auto& changes : symbol_change_list) {
            if(warnings::is_enabled(overlap_idem) && combined_changes.is_overlapping_and_not_idempotent(changes))
                warnings::warn(overlap_idem, "overlapping and non-idempotent changes in tocker-change application, will overwrite depending on the order:", conflict_string(combined_changes, changes));
            combined_changes += changes;
        }
        apply(combined_changes);
    }

    auto ntta_t::should_create_dependency_edge(const tta_t::graph_edge_iterator_t& e1, const tta_t::graph_edge_iterator_t& e2, expression_driver& i) const -> bool {
        if(e1->second.source == e2->second.source)
            return true;
        auto changes1 = i.evaluate(e1->second.data.updates);
        auto changes2 = i.evaluate(e2->second.data.updates);
        if(changes1.is_overlapping_and_not_idempotent(changes2)) {
            if(warnings::is_enabled(overlap_idem))
                warnings::warn(overlap_idem, "overlapping and non-idempotent changes in tick-change calculation:", conflict_string(changes1, changes2));
            return true;
        }
        return false;
    }

    auto ntta_t::calculate_edge_dependency_graph() -> tick_resolver::choice_dependency_problem {
        expression_driver i{symbols, external_symbols}; // BUG: external_symbols are not looked at yet
        tick_resolver::graph_type_builder graph_builder{};
        std::unordered_map<std::string, choice_t> all_enabled_choices{};
        uint32_t unique_counter = 0;
        for(auto component_it = components.begin(); component_it != components.end(); component_it++) {
            for(auto& edge : component_it->second.current_location->second.outgoing_edges) {
                if(!std::get<bool>(i.evaluate(edge->second.data.guard)))
                    continue;
                for(auto& n : graph_builder.nodes)
                    if(should_create_dependency_edge(edge, n.data, i))
                        graph_builder.add_edge(edge->first.identifier, n.data->first.identifier, unique_counter++);

                graph_builder.add_node({edge->first.identifier, edge});
                all_enabled_choices.insert({edge->first.identifier, choice_t{edge,{component_it,edge->second.target}, i.evaluate(edge->second.data.updates)}});
            }
        }
        try {
            return { graph_builder.validate().optimize().build(), all_enabled_choices };
        } catch (std::exception& e) {
            spdlog::critical("unable to generate enabled choice dependency graph: '{0}' please report this as an issue on github.com/sillydan1/AALTITOAD", e.what());
            throw;
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

    auto ntta_t::to_json() const -> nlohmann::json {
        nlohmann::json result{};
        result["locations"] = "[]"_json;
        for(auto& component : components) {
            auto j = "{}"_json;
            j[component.first] = component.second.current_location->first;
            result["locations"].push_back(j);
        }
        result["symbols"] = "{}"_json;
        for(auto& symbol : symbols + external_symbols) {
            std::visit(ya::overload(
                        [&result, &symbol](const int& v){ result["symbols"][symbol.first] = v; },
                        [&result, &symbol](const float& v){ result["symbols"][symbol.first] = v; },
                        [&result, &symbol](const bool& v){ result["symbols"][symbol.first] = v; },
                        [&result, &symbol](const std::string& v){ result["symbols"][symbol.first] = v; },
                        [&result, &symbol](const expr::clock_t& v){ result["symbols"][symbol.first] = v.time_units; },
                        [&result, &symbol](auto&& v){ result["symbols"][symbol.first] = "undefined"; }
                        ), static_cast<const expr::underlying_symbol_value_t&>(symbol.second));
        }
        return result;
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
