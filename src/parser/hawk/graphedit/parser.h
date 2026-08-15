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
#ifndef GEHAWK_PARSER_H
#define GEHAWK_PARSER_H
#include "model.h"
#include "parser/hawk/scoped_template_builder/model.h"
#include "plugin_system/parser.h"
#include <nlohmann/json.hpp>

namespace aaltitoad::hawk::graphedit {
    auto create_parser() -> plugin::parser*;

    struct compile_ok {
        std::vector<Diagnostic> diagnostics;
        tta_template tta;
    };

    struct compile_error {
        std::vector<Diagnostic> diagnostics;
    };

    using compile_result = std::expected<compile_ok, compile_error>;

    class parser : public plugin::parser {
    public:
        parser();
        ~parser() override = default;
        auto parse_files(const std::vector<std::string>& filepaths, const std::vector<std::string> &ignore_list) -> plugin::parse_result override;
        auto parse_model(const Buffer& buffer) -> plugin::parse_result override;
    private:
        auto should_ignore(const std::filesystem::directory_entry& entry, const std::vector<std::string>& ignore_list) -> bool;
        auto should_ignore(const std::filesystem::directory_entry& entry, const std::string& ignore_regex) -> bool;
        auto load_part(const nlohmann::json& json_file) -> std::string;

        auto to_graph(const Graph& g) -> graph_t;
        auto to_model(const Buffer& buffer) -> model_t;
        auto compile_type(const graphedit::location_type_t& type) -> urgency_t;
        auto compile_model(const model_t& model, const std::string& model_key) -> compile_result;
    };
}

#endif // !GEHAWK_PARSER_H
