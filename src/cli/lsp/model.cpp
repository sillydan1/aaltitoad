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
#include "spdlog/spdlog.h"

namespace aaltitoad::lsp {
    void from_json(const nlohmann::json& j, nail_t& t) {
        j.at("type").get_to(t.type);
        j.at("expression").at(1).at("message").get_to(t.expression);
        j.at("nickname").at(1).at("message").get_to(t.nickname);
    }

    void from_json(const nlohmann::json& j, location_t& t) {
        j.at("type").get_to(t.type);
        j.at("nickname").at(1).at("message").get_to(t.nickname);
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
        spdlog::info("bad j.at(0): {}", j.at(0));
    }
}
