#ifndef AALTITOAD_MODEL_H
#define AALTITOAD_MODEL_H
#include <nlohmann/json.hpp>

namespace aaltitoad::hawk::model {
    enum class urgency_t {
        normal, urgent, committed, invalid=-1
    };

    struct location_t {
        std::string id;
        std::string nickname;
        std::string invariant;
        urgency_t urgency{urgency_t::invalid};
    };

    struct edge_t {
        std::string id;
        std::string source;
        std::string target;
        std::string guard;
        std::string update;
    };

    struct tta_instance_t {
        std::string id;
        std::string tta_template_name;
        std::string invocation;
    };

    struct tta_template {
        std::string name;
        std::string declarations;
        bool is_main;
        std::vector<location_t> locations;
        std::vector<edge_t> edges;
        location_t initial_location;
        location_t final_location;
        std::vector<tta_instance_t> instances;
    };

    struct part_t {
        std::string id;
        std::string value;
    };

    void from_json(const nlohmann::json& j, urgency_t& l);
    void from_json(const nlohmann::json& j, location_t& l);
    void from_json(const nlohmann::json& j, edge_t& e);
    void from_json(const nlohmann::json& j, tta_instance_t& i);
    void from_json(const nlohmann::json& j, tta_template& t);
    void from_json(const nlohmann::json& j, part_t& p);
}

#endif //AALTITOAD_MODEL_H
