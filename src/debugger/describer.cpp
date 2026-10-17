/**
 * aaltitoad - a verification engine for tick tock automata models
   Copyright (C) 2026 Asger Gitz-Johansen

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
#include "describer.h"
#include <algorithm>
#include <charconv>
#include <variant>

namespace aaltitoad::debugger {
    namespace {
        auto base(const expr::symbol_value_t& value) -> const expr::underlying_symbol_value_t& {
            return value;
        }

        auto variable_json(const expr::symbol_value_t& value) -> nlohmann::json {
            return {{"value", value_to_json(value)}, {"type", type_of(value)}, {"writable", true}};
        }
    }

    auto type_of(const expr::symbol_value_t& value) -> std::string {
        const auto& v = base(value);
        if(std::holds_alternative<int>(v))
            return "int";
        if(std::holds_alternative<float>(v))
            return "float";
        if(std::holds_alternative<bool>(v))
            return "bool";
        if(std::holds_alternative<std::string>(v))
            return "string";
        return "clock";
    }

    auto value_to_json(const expr::symbol_value_t& value) -> nlohmann::json {
        const auto& v = base(value);
        if(const auto* i = std::get_if<int>(&v))
            return *i;
        if(const auto* f = std::get_if<float>(&v)) {
            // NOTE: the shortest text that is the float (0.1f is 0.1, not 0.10000000149011612)
            char buffer[64]{};
            auto [end, error] = std::to_chars(buffer, buffer + sizeof(buffer), *f);
            return error == std::errc{} ? std::stod(std::string{buffer, end}) : static_cast<double>(*f);
        }
        if(const auto* b = std::get_if<bool>(&v))
            return *b;
        if(const auto* s = std::get_if<std::string>(&v))
            return *s;
        return std::get<expr::clock_t>(v).time_units;
    }

    describer::describer(const graphedit::network_t& network) : network_{network} {
        for(auto& [name, instance] : network_.instances)
            for(auto& local : instance.local_identifiers)
                private_symbols_[name + "." + local] = {name, local};
    }

    auto describer::state(const ntta_t& state) const -> nlohmann::json {
        auto variables = nlohmann::json::object();
        std::map<std::string, nlohmann::json> instance_variables{};
        for(auto& [name, value] : state.symbols) {
            auto variable = variable_of(name);
            if(variable.instance.empty())
                variables[name] = variable_json(value);
            else
                instance_variables[variable.instance][variable.name] = variable_json(value);
        }
        for(auto& [name, value] : state.external_symbols)
            variables[name] = variable_json(value);
        auto instances = nlohmann::json::array();
        for(auto& [name, instance] : network_.instances) {
            auto variables_of_instance = instance_variables.contains(name) ? instance_variables.at(name) : nlohmann::json::object();
            instances.push_back({{"name", name}, {"model", instance.model},
                                 {"active", {state.components.at(name).current_location->first}},
                                 {"variables", variables_of_instance}});
        }
        return {{"instances", instances}, {"variables", variables}};
    }

    auto describer::variable_of(const std::string& symbol) const -> variable_name_t {
        auto it = private_symbols_.find(symbol);
        return it == private_symbols_.end() ? variable_name_t{"", symbol} : it->second;
    }

    auto describer::is_private(const std::string& symbol) const -> bool {
        return private_symbols_.contains(symbol);
    }

    auto describer::edges_of(const std::vector<std::string>& edges) const -> std::vector<const graphedit::instance_edge_t*> {
        std::vector<const graphedit::instance_edge_t*> result{};
        for(auto& identifier : edges)
            if(auto edge = network_.edges.find(identifier); edge != network_.edges.end())
                result.push_back(&edge->second);
        std::sort(result.begin(), result.end(), [](const auto* a, const auto* b) {
            return std::tie(a->instance, a->edge.id) < std::tie(b->instance, b->edge.id);
        });
        return result;
    }

    auto describer::elements_of(const std::vector<std::string>& edges) const -> nlohmann::json {
        auto elements = nlohmann::json::array();
        std::map<std::string, std::size_t> index_of{};
        for(const auto* edge : edges_of(edges)) {
            if(!index_of.contains(edge->instance)) {
                index_of[edge->instance] = elements.size();
                elements.push_back({{"instance", edge->instance}, {"model", edge->model}, {"ids", nlohmann::json::array()}});
            }
            auto& ids = elements[index_of.at(edge->instance)]["ids"];
            for(auto& element : edge->edge.elements)
                ids.push_back(element);
        }
        return elements;
    }

    auto describer::location_name(const std::string& model, const std::string& location) const -> std::string {
        const auto* found = network_.models.at(model).find_location(location);
        if(found == nullptr)
            return "a location";
        if(!found->nickname.empty())
            return "'" + found->nickname + "'";
        switch(found->kind) {
            case graphedit::location_kind_t::initial: return "the initial location";
            case graphedit::location_kind_t::final: return "the final location";
            default: return "a location";
        }
    }

    auto describer::label(const std::vector<std::string>& edges, const std::set<const graphedit::instance_edge_t*>& common,
                          bool detailed) const -> std::string {
        constexpr std::size_t max_expression = 40;
        auto shortened = [](const std::string& text) {
            return text.size() <= max_expression ? text : text.substr(0, max_expression - 3) + "...";
        };
        auto short_name = [this](const std::string& model, const std::string& location) -> std::string {
            const auto* found = network_.models.at(model).find_location(location);
            if(found == nullptr)
                return "?";
            if(!found->nickname.empty())
                return found->nickname;
            return found->kind == graphedit::location_kind_t::initial ? "initial"
                 : found->kind == graphedit::location_kind_t::final ? "final" : "location";
        };
        std::vector<std::string> parts{};
        std::size_t omitted = 0;
        for(const auto* edge : edges_of(edges)) {
            if(common.contains(edge)) {
                omitted++;
                continue;
            }
            auto part = edge->instance + ": " + short_name(edge->model, edge->edge.source) + " → " +
                        short_name(edge->model, edge->edge.target);
            if(detailed && !edge->edge.guard.empty())
                part += " if " + shortened(edge->edge.guard);
            else if(detailed && !edge->edge.update.empty())
                part += " do " + shortened(edge->edge.update);
            parts.push_back(part);
        }
        if(parts.empty())
            return omitted > 0 ? label(edges, {}, detailed) : "tick";
        std::string result{};
        for(std::size_t i = 0; i < parts.size() && i < max_label_edges; i++)
            result += (i == 0 ? "" : ", ") + parts[i];
        if(parts.size() > max_label_edges)
            result += " and " + std::to_string(parts.size() - max_label_edges) + " more";
        if(omitted == 1)
            result += " (and the edge that every step takes)";
        else if(omitted > 1)
            result += " (and the " + std::to_string(omitted) + " edges that every step takes)";
        return result;
    }

    auto describer::input_label(const expr::symbol_table_t& changes) const -> std::string {
        if(changes.empty())
            return "the environment";
        std::string result{};
        for(auto& [name, value] : changes)
            result += (result.empty() ? "" : ", ") + name + " = " + value_to_json(value).dump();
        return result;
    }
}
