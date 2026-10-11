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
#include "ntta_model.h"
#include "problems.h"
#include <algorithm>
#include <map>
#include <set>
#include <stdexcept>

namespace aaltitoad::graphedit {
    namespace {
        auto string_value(const nlohmann::json& object, const char* key, const std::string& fallback = {}) -> std::string {
            if(!object.is_object())
                return fallback;
            auto it = object.find(key);
            if(it == object.end() || !it->is_string())
                return fallback;
            return it->get<std::string>();
        }

        auto bool_value(const nlohmann::json& object, const char* key, bool fallback) -> bool {
            if(!object.is_object())
                return fallback;
            auto it = object.find(key);
            if(it == object.end())
                return fallback;
            if(it->is_boolean())
                return it->get<bool>();
            if(it->is_string())
                return it->get<std::string>() == "true";
            return fallback;
        }

        auto trimmed(const std::string& text) -> std::string {
            auto begin = text.find_first_not_of(" \t\r\n");
            if(begin == std::string::npos)
                return {};
            auto end = text.find_last_not_of(" \t\r\n");
            return text.substr(begin, end - begin + 1);
        }

        auto lowercase(std::string text) -> std::string {
            std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c){ return std::tolower(c); });
            return text;
        }

        auto location_kind(const std::string& text) -> location_kind_t {
            if(text == "initial")
                return location_kind_t::initial;
            if(text == "final")
                return location_kind_t::final;
            return location_kind_t::normal;
        }

        auto nail_kind(const std::string& text) -> nail_kind_t {
            if(text == "guard")
                return nail_kind_t::guard;
            if(text == "update")
                return nail_kind_t::update;
            return nail_kind_t::comment;
        }

        // Collects the problems of a model, once each
        class problem_collector {
        public:
            explicit problem_collector(std::vector<problem_t>& problems) : problems{problems} {}
            void add(problem_t problem) {
                if(std::find(problems.begin(), problems.end(), problem) == problems.end())
                    problems.push_back(std::move(problem));
            }
        private:
            std::vector<problem_t>& problems;
        };

        auto broken_edge(const std::string& message, std::vector<std::string> elements) -> problem_t {
            return {problems::broken_edge, severity_t::error, "An edge does not arrive at a location", message,
                    std::move(elements)};
        }

        // Follow the connections from a location through nails to the next location
        void derive_edges(ntta_model_t& model) {
            problem_collector problems{model.problems};
            std::map<std::string, std::vector<const connection_t*>> outgoing{};
            for(auto& connection : model.connections)
                outgoing[connection.source].push_back(&connection);
            for(auto& connection : model.connections) {
                if(model.find_subcomponent(connection.source) != nullptr || model.find_subcomponent(connection.target) != nullptr) {
                    problems.add({problems::subcomponent_connection, severity_t::error,
                                  "aaltitoad does not run connections into or out of subcomponents",
                                  "aaltitoad runs subcomponents in parallel with their template from the start: edges "
                                  "that enter or leave subcomponents (the sequential composition of H-UPPAAL) are not "
                                  "supported",
                                  {connection.id}});
                    continue;
                }
                if(model.find_location(connection.source) == nullptr)
                    continue;
                edge_t edge{.id=connection.id, .source=connection.source, .target={}, .guard={}, .update={},
                            .elements={connection.id}, .guard_nails={}, .update_nails={}};
                std::vector<std::string> guards{};
                std::vector<std::string> updates{};
                std::set<std::string> visited{};
                auto current = connection.target;
                auto arrived = false;
                while(true) {
                    if(model.find_location(current) != nullptr) {
                        arrived = true;
                        break;
                    }
                    if(model.find_subcomponent(current) != nullptr)
                        break; // reported above
                    const auto* nail = model.find_nail(current);
                    if(nail == nullptr) {
                        problems.add(broken_edge("A connection leads to a vertex that does not exist", {edge.elements.back()}));
                        break;
                    }
                    if(!visited.insert(current).second) {
                        problems.add(broken_edge("The nails of the edge lead into each other in a circle", {visited.begin(), visited.end()}));
                        break;
                    }
                    edge.elements.push_back(current);
                    auto expression = trimmed(nail->expression);
                    if(nail->kind == nail_kind_t::guard && !expression.empty()) {
                        guards.push_back(expression);
                        edge.guard_nails.push_back(nail->id);
                    }
                    if(nail->kind == nail_kind_t::update && !expression.empty()) {
                        updates.push_back(expression);
                        edge.update_nails.push_back(nail->id);
                    }
                    auto next = outgoing.find(current);
                    if(next == outgoing.end() || next->second.empty()) {
                        problems.add(broken_edge("The nail leads nowhere", {current}));
                        break;
                    }
                    if(next->second.size() > 1) {
                        problems.add(broken_edge("The nail leads to several vertices", {current}));
                        break;
                    }
                    edge.elements.push_back(next->second.front()->id);
                    current = next->second.front()->target;
                }
                if(!arrived)
                    continue;
                edge.target = current;
                if(guards.size() == 1)
                    edge.guard = guards.front();
                for(std::size_t i = 0; guards.size() > 1 && i < guards.size(); i++)
                    edge.guard += (i == 0 ? "(" : " && (") + guards[i] + ")";
                for(std::size_t i = 0; i < updates.size(); i++)
                    edge.update += (i == 0 ? "" : "; ") + updates[i];
                model.edges.push_back(std::move(edge));
            }
        }
    }

    auto ntta_model_t::is_template() const -> bool {
        return !locations.empty() || !nails.empty() || !subcomponents.empty();
    }

    auto ntta_model_t::find_location(const std::string& id) const -> const location_t* {
        auto it = std::find_if(locations.begin(), locations.end(), [&id](const auto& l){ return l.id == id; });
        return it == locations.end() ? nullptr : &*it;
    }

    auto ntta_model_t::find_nail(const std::string& id) const -> const nail_t* {
        auto it = std::find_if(nails.begin(), nails.end(), [&id](const auto& n){ return n.id == id; });
        return it == nails.end() ? nullptr : &*it;
    }

    auto ntta_model_t::find_subcomponent(const std::string& id) const -> const subcomponent_t* {
        auto it = std::find_if(subcomponents.begin(), subcomponents.end(), [&id](const auto& s){ return s.id == id; });
        return it == subcomponents.end() ? nullptr : &*it;
    }

    auto ntta_model_t::edges_through(const std::string& element) const -> std::vector<const edge_t*> {
        std::vector<const edge_t*> result{};
        for(auto& edge : edges)
            if(std::find(edge.elements.begin(), edge.elements.end(), element) != edge.elements.end())
                result.push_back(&edge);
        return result;
    }

    auto is_ntta_model(const nlohmann::json& metadata) -> bool {
        return string_value(metadata, "graphedit_syntax") == syntax_name;
    }

    auto read_model(const std::string& key, const nlohmann::json& metadata, const nlohmann::json& graph) -> ntta_model_t {
        if(!graph.is_object())
            throw std::invalid_argument(key + ": the graph is not a json object");
        ntta_model_t result{};
        result.key = key;
        result.name = trimmed(string_value(metadata, "name"));
        result.is_main = lowercase(trimmed(string_value(metadata, "is_main"))) == "true";
        result.declarations = string_value(graph, "declarations");
        auto vertices = graph.find("vertices");
        if(vertices != graph.end() && vertices->is_object()) {
            for(auto& [id, vertex] : vertices->items()) {
                if(!vertex.is_object())
                    throw std::invalid_argument(key + ": the vertex " + id + " is not a json object");
                auto type = string_value(vertex, "type");
                auto properties = vertex.contains("properties") ? vertex.at("properties") : nlohmann::json::object();
                if(type == "location")
                    result.locations.push_back({id, string_value(properties, "nickname"),
                                                location_kind(string_value(properties, "kind", "normal")),
                                                bool_value(properties, "immediate", false)});
                else if(type == "nail")
                    result.nails.push_back({id, nail_kind(string_value(properties, "kind", "comment")),
                                            string_value(properties, "expression")});
                else if(type == "subcomponent")
                    result.subcomponents.push_back({id, trimmed(string_value(properties, "template")),
                                                    trimmed(string_value(properties, "invocation"))});
            }
        }
        auto edges = graph.find("edges");
        if(edges != graph.end() && edges->is_object()) {
            for(auto& [id, edge] : edges->items()) {
                if(!edge.is_object())
                    throw std::invalid_argument(key + ": the edge " + id + " is not a json object");
                if(string_value(edge, "type") == "connection")
                    result.connections.push_back({id, string_value(edge, "source"), string_value(edge, "target")});
            }
        }
        derive_edges(result);
        return result;
    }

    auto read_buffer(const nlohmann::json& buffer) -> ntta_model_t {
        if(!buffer.is_object() || !buffer.contains("key") || !buffer.at("key").is_string())
            throw std::invalid_argument("a buffer needs a key");
        auto key = buffer.at("key").get<std::string>();
        if(!buffer.contains("graph"))
            throw std::invalid_argument(key + ": the buffer has no graph");
        return read_model(key, buffer.contains("metadata") ? buffer.at("metadata") : nlohmann::json::object(),
                          buffer.at("graph"));
    }

    auto read_model_file(const std::string& key, const nlohmann::json& document) -> std::optional<ntta_model_t> {
        if(!document.is_object() || !document.contains("formatVersion") || !document.contains("syntax"))
            return std::nullopt;
        auto version = document.at("formatVersion");
        if(!version.is_number_integer() || version.get<int>() != 2)
            return std::nullopt;
        auto metadata = document.contains("metadata") ? document.at("metadata") : nlohmann::json::object();
        if(!is_ntta_model(metadata))
            return std::nullopt;
        return read_model(key, metadata, document.at("syntax"));
    }

    auto is_legacy_model_file(const nlohmann::json& document) -> bool {
        if(!document.is_object() || document.contains("formatVersion"))
            return false;
        auto metadata = document.find("metadata");
        if(metadata == document.end())
            return false;
        // graphedit 1.x wrote type information around its values: "metadata": ["java.util.HashMap", {...}]
        auto values = metadata->is_array() && metadata->size() == 2 ? metadata->at(1) : *metadata;
        auto syntax = string_value(values, "graphedit_syntax");
        return syntax == "hawk" || syntax == syntax_name;
    }
}
