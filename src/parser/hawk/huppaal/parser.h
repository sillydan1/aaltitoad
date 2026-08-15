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
#ifndef AALTITOAD_HAWK_PARSER_H
#define AALTITOAD_HAWK_PARSER_H
#include "plugin_system/parser.h"
#include "parser/hawk/compiler.h"
#include <nlohmann/json.hpp>

namespace aaltitoad::hawk::huppaal {
    auto create_parser() -> plugin::parser*;

    class huppaal_scanner : public aaltitoad::hawk::scanner {
    public:
        ~huppaal_scanner() override = default;
        auto scan(compiler& ctx,
                const std::vector<std::string>& filepaths,
                const std::vector<std::string>& ignore_list) const noexcept -> std::expected<scanner::ok, error_t> override;
    private:
        auto should_ignore(const std::filesystem::directory_entry& entry, const std::vector<std::string>& ignore_list) const -> bool;
        auto should_ignore(const std::filesystem::directory_entry& entry, const std::string& ignore_regex) const -> bool;

        auto scan_template(const std::string& filepath, const nlohmann::json& t) const -> scanning::template_t;
        auto scan_parts(const nlohmann::json& t) const -> std::string;
    };

    class huppaal_parser : public aaltitoad::hawk::parser {
    public:
        ~huppaal_parser() override = default;
        auto parse(compiler& ctx, const scanner::ok& stream) const noexcept -> std::expected<parser::ok, error_t> override;
    };

    class huppaal_semantic_analyzer : public aaltitoad::hawk::semantic_analyzer {
    public:
        ~huppaal_semantic_analyzer() override = default;
        auto analyze(compiler& ctx, const parser::ok& ast) const noexcept -> std::expected<parser::ok, error_t> override;
    };

    class huppaal_generator : public aaltitoad::hawk::generator {
    public:
        ~huppaal_generator() override = default;
        auto generate(compiler& ctx, const parser::ok& ast) const noexcept -> std::expected<ntta_t, error_t> override;
    };

    class parser : public plugin::parser {
    public:
        ~parser() override = default;
        auto parse_files(const std::vector<std::string>& files, const std::vector<std::string>& ignore_patterns) -> plugin::parse_result override;
        auto parse_model(const Buffer& buffer) -> plugin::parse_result override;
    private:
        auto create_compiler() -> compiler;

        huppaal_scanner _scanner;
        huppaal_parser _parser;
        huppaal_semantic_analyzer _semantic_analyzer;
        nothing_optimizer _optimizer;
        huppaal_generator _generator;
    };
}

#endif //AALTITOAD_HAWK_PARSER_H
