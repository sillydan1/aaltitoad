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
#ifndef AALTITOAD_MODEL_H
#define AALTITOAD_MODEL_H
#include <nlohmann/json.hpp>

namespace aaltitoad::hawk {
    enum class urgency_t {
        normal, urgent, committed, invalid=-1
    };

    struct location_t {
        std::string id;
        std::string nickname;
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
