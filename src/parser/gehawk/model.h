#ifndef GEHAWK_MODEL_H
#define GEHAWK_MODEL_H
#include <optional>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace aaltitoad::gehawk::model {
    enum class location_type_t {
        normal=0,
        initial,
        final,
        invalid=-1
    };

    enum class immediacy_t {
        normal=0,
        immediate,
        invalid=-1
    };

    enum class nail_type_t {
        guard=0,
        update,
        comment,
        invalid=-1,
    };

    struct location_t {
        location_type_t type;
        std::optional<std::string> nickname;
        immediacy_t immediacy;
    };

    struct nail_t {
        nail_type_t type;
        std::string expression;
        std::optional<std::string> nickname;
    };

    struct tta_instantiation_t {
        std::string template_name;
    };

    struct edge_t {
        std::string source;
        std::string target;
    };

    struct tta_t {
        std::string name;
        std::vector<location_t> locations;
        std::vector<nail_t> nails;
        std::vector<edge_t> edges;
        std::vector<tta_instantiation_t> instantiations;
    };

    void from_json(const nlohmann::json& j, location_t& t);
    void from_json(const nlohmann::json& j, nail_t& t);
    void from_json(const nlohmann::json& j, edge_t& t);
    void from_json(const nlohmann::json& j, tta_instantiation_t& t);
    void from_json(const nlohmann::json& j, tta_t& t);
}

#endif // !GEHAWK_MODEL_H
