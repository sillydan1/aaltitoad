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
#include <drivers/driver.h>
#include <nlohmann/json.hpp>
#include "query_json_loader.h"

namespace aaltitoad {
    auto load_query_json_file(const std::string& json_file, std::initializer_list<expr::symbol_table_ref_t> environments) -> std::vector<ctl::compiler::compiled_expr_t> {
        try {
            std::vector<ctl::compiler::compiled_expr_t> result{};
            ctl::compiler c{environments};
            std::ifstream f(json_file);
            auto data = nlohmann::json::parse(f);
            for(auto& q : data) {
                spdlog::trace("compiling query {0}", q["query"]);
                result.emplace_back(c.compile(q["query"]));
            }
            return result;
        } catch (std::exception &e) {
            spdlog::error("unable to parse json file {0}: {1}", json_file, e.what());
            throw e;
        }
    }
}
