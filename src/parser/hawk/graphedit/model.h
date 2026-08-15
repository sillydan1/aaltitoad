#ifndef GEHAWK_MODEL_H
#define GEHAWK_MODEL_H
#include <nlohmann/json.hpp>
#include <string>
#include <variant>

namespace aaltitoad::hawk::graphedit {
    enum class location_type_t {
        normal=0,
        initial,
        final,
        immediate,
        invalid=-1
    };

    enum class nail_type_t {
        guard=0,
        update,
        comment,
        invalid=-1,
    };

    enum class edge_type_t {
        normal=0,
        box_edge,
        invalid=-1,
    };

    struct location_t {
        location_type_t type;
        std::string nickname;
    };

    struct nail_t {
        nail_type_t type;
        std::string expression;
        std::string nickname;
    };

    struct instantiation_t {
        std::string template_name;
    };

    using vertex_t = std::variant<location_t, nail_t, instantiation_t>;

    struct edge_t {
        edge_type_t type;
        std::string source;
        std::string target;
    };

    struct graph_t {
        std::string declarations; 
        std::unordered_map<std::string, vertex_t> vertices;
        std::unordered_map<std::string, edge_t> edges;
    };

    struct model_t {
        std::unordered_map<std::string, std::string> metadata;
        graph_t syntax;
    };

    void from_json(const nlohmann::json& j, location_type_t& t);
    void from_json(const nlohmann::json& j, nail_type_t& t);
    void from_json(const nlohmann::json& j, location_t& t);
    void from_json(const nlohmann::json& j, nail_t& t);
    void from_json(const nlohmann::json& j, instantiation_t& t);
    void from_json(const nlohmann::json& j, vertex_t& t);
    void from_json(const nlohmann::json& j, edge_t& t);
    void from_json(const nlohmann::json& j, graph_t& t);
    void from_json(const nlohmann::json& j, model_t& t);
}

#endif // !GEHAWK_MODEL_H
