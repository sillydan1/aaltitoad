#include "model.h"

namespace aaltitoad::hawk::graphedit {
    void from_json(const nlohmann::json& j, location_type_t& t) {
        std::string type_str{j};
        if(type_str == "NORMAL")
            t = location_type_t::normal;
        else if(type_str == "IMMEDIATE")
            t = location_type_t::immediate;
        else if(type_str == "INITIAL")
            t = location_type_t::initial;
        else if(type_str == "FINAL")
            t = location_type_t::final;
        else
            t = location_type_t::invalid;
    }

    void from_json(const nlohmann::json& j, nail_type_t& t) {
        std::string type_str{j};
        if(type_str == "GUARD")
            t = nail_type_t::guard;
        else if(type_str == "UPDATE")
            t = nail_type_t::update;
        else if(type_str == "COMMENT")
            t = nail_type_t::comment;
        else
            t = nail_type_t::invalid;
    }

    void from_json(const nlohmann::json& j, location_t& t) {
        j.at("type").get_to(t.type);
        j.at("nickname").at(1).at("message").get_to(t.nickname);
    }

    void from_json(const nlohmann::json& j, nail_t& t) {
        j.at("type").get_to(t.type);
        j.at("expression").at(1).at("message").get_to(t.expression);
        j.at("nickname").at(1).at("message").get_to(t.nickname);
    }

    void from_json(const nlohmann::json& j, instantiation_t& t) {
        j.at("templateName").get_to(t.template_name);
    }

    void from_json(const nlohmann::json& j, vertex_t& t) {
        std::string class_name = j.at(0);
        if(class_name.find("ModelNail") != std::string::npos) {
            t = nail_t();
            j.at(1).get_to(std::get<nail_t>(t));
            return;
        }
        if(class_name.find("ModelLocation") != std::string::npos) {
            t = location_t();
            j.at(1).get_to(std::get<location_t>(t));
            return;
        }
        if(class_name.find("ModelInstantiation") != std::string::npos) {
            t = instantiation_t();
            j.at(1).get_to(std::get<instantiation_t>(t));
            return;
        }
        spdlog::info("unrecognized vertex type: {}", class_name);
    }

    void from_json(const nlohmann::json& j, edge_t& t) {
        std::string class_name = j.at(0);
        if(class_name.find("ModelBoxEdge") != std::string::npos) {
            t.type = edge_type_t::box_edge;
            j.at(1).at("source").get_to(t.source);
            j.at(1).at("target").get_to(t.target);
            return;
        }
        if(class_name.find("ModelConnection") != std::string::npos) {
            t.type = edge_type_t::normal;
            j.at(1).at("source").get_to(t.source);
            j.at(1).at("target").get_to(t.target);
            return;
        }
        t.type = edge_type_t::invalid;
        spdlog::info("unrecognized edge type: {}", class_name);
    }

    void from_json(const nlohmann::json& j, graph_t& t) {
        j.at("declarations").get_to(t.declarations);
        j.at("vertices").at(1).get_to(t.vertices);
        j.at("edges").at(1).get_to(t.edges);
    }

    void from_json(const nlohmann::json& j, model_t& t) {
        j.at("metadata").at(1).get_to(t.metadata);
        j.at("syntax").get_to(t.syntax);
    }
}
