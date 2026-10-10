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
#ifndef AALTITOAD_GRAPHEDIT_NTTA_MODEL_H
#define AALTITOAD_GRAPHEDIT_NTTA_MODEL_H
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <vector>

// Models of the NTTA syntax of graphedit (https://git.gtz.dk/gtz-agent/graphedit-ntta), as graphedit writes them
// (model files of format version 2) and sends them to language servers (buffers). Every model is a template of a tick
// tock automaton; models without vertices only declare symbols.
namespace aaltitoad::graphedit {
    // The name of the syntax in the metadata of models (graphedit_syntax)
    inline constexpr auto syntax_name = "NTTA";

    enum class severity_t { hint, info, warning, error };

    // Something wrong with a model, a diagnostic of the graph language server protocol of graphedit
    struct problem_t {
        // identifies the kind of problem, e.g. "AALTITOAD-E004"
        std::string identifier;
        severity_t severity;
        std::string title;
        std::string message;
        // the vertices and edges that the problem is about
        std::vector<std::string> elements;
        auto operator==(const problem_t&) const -> bool = default;
    };

    enum class location_kind_t { normal, initial, final };
    enum class nail_kind_t { guard, update, comment };

    struct location_t {
        std::string id;
        std::string nickname;
        location_kind_t kind;
        bool immediate;
    };

    // A point of an edge that carries a guard, an update or a comment
    struct nail_t {
        std::string id;
        nail_kind_t kind;
        std::string expression;
    };

    // An instance of a template, e.g. the template "Light(id)" with the invocation "lamp(1)"
    struct subcomponent_t {
        std::string id;
        std::string template_name;
        std::string invocation;
    };

    // A part of an edge: from a location, nail or subcomponent to the next one
    struct connection_t {
        std::string id;
        std::string source;
        std::string target;
    };

    // An edge from a location to a location: a chain of connections through nails. Several edges may share the end of
    // their chain (several connections may lead into the same nail).
    struct edge_t {
        // the id of the first connection, which only this edge starts with
        std::string id;
        // the locations that the edge leads from and to
        std::string source;
        std::string target;
        // the conjunction of the guards of its nails (empty: always enabled)
        std::string guard;
        // the updates of its nails, in their order (empty: none)
        std::string update;
        // its connections and nails, in order
        std::vector<std::string> elements;
        // the nails whose expressions make up the guard and the update
        std::vector<std::string> guard_nails;
        std::vector<std::string> update_nails;
    };

    // A model of the NTTA syntax
    struct ntta_model_t {
        // the path of the model file relative to the project directory
        std::string key;
        // the name of the template, with its parameters (e.g. "Light(id)")
        std::string name;
        bool is_main{false};
        std::string declarations;
        std::vector<location_t> locations;
        std::vector<nail_t> nails;
        std::vector<subcomponent_t> subcomponents;
        std::vector<connection_t> connections;
        // the edges that the connections make up
        std::vector<edge_t> edges;
        // what is wrong with the structure of the model (edges that do not arrive at a location, connections into or
        // out of subcomponents)
        std::vector<problem_t> problems;

        // Whether the model is a template; models without vertices only declare symbols
        auto is_template() const -> bool;
        auto find_location(const std::string& id) const -> const location_t*;
        auto find_nail(const std::string& id) const -> const nail_t*;
        auto find_subcomponent(const std::string& id) const -> const subcomponent_t*;
        // The edges that consist of the element (a connection or a nail)
        auto edges_through(const std::string& element) const -> std::vector<const edge_t*>;
    };

    // Whether the metadata of a model says that it is an NTTA model
    auto is_ntta_model(const nlohmann::json& metadata) -> bool;
    // Read a model from its metadata and its graph ({"declarations", "vertices", "edges"}). Missing properties have their
    // default values. Throws std::invalid_argument if the json is not a graph.
    auto read_model(const std::string& key, const nlohmann::json& metadata, const nlohmann::json& graph) -> ntta_model_t;
    // Read a buffer of the graph language server protocol ({"key", "metadata", "graph"}).
    // Throws std::invalid_argument if it is not a buffer.
    auto read_buffer(const nlohmann::json& buffer) -> ntta_model_t;
    // Read a model file of graphedit. Returns std::nullopt if the document is not an NTTA model of the current file
    // format (another syntax, or not a model). Throws std::invalid_argument if the json of the model is broken.
    auto read_model_file(const std::string& key, const nlohmann::json& document) -> std::optional<ntta_model_t>;
    // Whether a document is a model file of graphedit 1.x (which graphedit converts when it opens it, but aaltitoad
    // cannot read)
    auto is_legacy_model_file(const nlohmann::json& document) -> bool;
}

#endif //AALTITOAD_GRAPHEDIT_NTTA_MODEL_H
