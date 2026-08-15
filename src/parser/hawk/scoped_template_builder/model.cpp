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
#include "model.h"
#include <uuid>

namespace aaltitoad::hawk {
    void from_json(const nlohmann::json& j, urgency_t& e) {
        static const std::pair<urgency_t, nlohmann::json> m[] = {
                {urgency_t::invalid, nullptr},
                {urgency_t::normal, "NORMAL"},
                {urgency_t::urgent, "URGENT"},
                {urgency_t::committed, "COMMITTED"},
        };
        auto it = std::find_if(std::begin(m), std::end(m),
                               [&j](const std::pair<urgency_t, nlohmann::json>& ej_pair) { return ej_pair.second == j; });
        e = ((it != std::end(m)) ? it : std::begin(m))->first;
    }

    void from_json(const nlohmann::json& j, location_t& l) {
        j.at("id").get_to(l.id);
        j.at("nickname").get_to(l.nickname);
        j.at("urgency").get_to(l.urgency);
    }

    void from_json(const nlohmann::json& j, edge_t& e) {
        if(j.contains("uuid"))
            j.at("uuid").get_to(e.id);
        else
            e.id = ya::uuid_v4_custom("E", "");

        if(j.contains("source_location"))
            j.at("source_location").get_to(e.source);
        else
            j.at("source_sub_component").get_to(e.source);

        if(j.contains("target_location"))
            j.at("target_location").get_to(e.target);
        else
            j.at("target_sub_component").get_to(e.target);

        j.at("guard").get_to(e.guard);
        j.at("update").get_to(e.update);
    }

    void from_json(const nlohmann::json& j, tta_instance_t& i) {
        if(j.contains("uuid"))
            j.at("uuid").get_to(i.id);
        else
            i.id = ya::uuid_v4_custom("I", "");
        j.at("component").get_to(i.tta_template_name);
        j.at("identifier").get_to(i.invocation);
    }

    void from_json(const nlohmann::json& j, tta_template& t) {
        j.at("name").get_to(t.name);
        j.at("declarations").get_to(t.declarations);
        j.at("main").get_to(t.is_main);
        j.at("locations").get_to(t.locations);
        j.at("edges").get_to(t.edges);
        j.at("initial_location").get_to(t.initial_location);
        j.at("final_location").get_to(t.final_location);
        j.at("sub_components").get_to(t.instances);
    }

    void from_json(const nlohmann::json& j, part_t& p) {
        j.at("ID").get_to(p.id);
        if(j.contains("Value")) {
            p.value = (std::stringstream{} << j.at("Value")).str();
        } else {
            auto type = j.at("Type").get<std::string>();
            if(type == "EMR")                 p.value = "0"; // EMR type is being phased out, but this is fine for now
            if(type == "Timer")               p.value = "0_ms";
            if(type == "DigitalOutput")       p.value = "false";
            if(type == "DigitalInput")        p.value = "false";
            if(type == "DigitalToggleSwitch") p.value = "false";
            if(type == "HighSpeedCounter")    p.value = "0";
            if(type == "AnalogInput")         p.value = "0.0";
            if(type == "AnalogOutput")        p.value = "0.0";
        }
    }
}
