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
#ifndef LSP_MODEL_H
#define LSP_MODEL_H
#include <nlohmann/json.hpp>
#include <variant>

namespace aaltitoad::lsp {
    struct position_t {
        double x;
        double y;
    };

    struct nail_t {
        position_t position;
        std::string type;
        std::string expression;
        std::string nickname;
    };

    struct location_t {
        position_t position;
        std::string type;
        std::string nickname;
    };

    using vertex_t = std::variant<location_t, nail_t>;

    void from_json(const nlohmann::json& j, position_t& t);
    void from_json(const nlohmann::json& j, nail_t& t);
    void from_json(const nlohmann::json& j, location_t& t);
    void from_json(const nlohmann::json& j, vertex_t& t);
}

#endif // !LSP_MODEL_H
