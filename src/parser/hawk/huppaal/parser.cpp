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
#include "parser.h"
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include "lsp.pb.h"
#include "plugin_system/plugin_system.h"

namespace aaltitoad::hawk::huppaal {
    diagnostic json_error(const std::string& filepath, const std::string& err) {
        return {
            .identifier="json_error",
            .title="JSON error in file: " + filepath,
            .message="unable to parse json: " + err,
            .description="JSON model files must be of a specific format", // TODO: jsomschema validation
            .severity=Severity::SEVERITY_ERROR
        };
    }

    static diagnostic no_main{
        .identifier="no_main",
        .title="No main template",
        .message="Could not find a template marked as \"main\"",
        .description="A valid network of TTAs must have at least one template marked as the main template",
        .severity=Severity::SEVERITY_ERROR
    };

    /* =============================== */

    auto any_errors(const std::vector<Diagnostic>& ds) -> bool {
        for(auto& d : ds)
            if(d.severity() == Severity::SEVERITY_ERROR)
                return true;
        return false;
    }

    auto huppaal_scanner::scan(compiler& ctx, const std::vector<std::string>& filepaths, const std::vector<std::string>& ignore_list) const noexcept -> std::expected<scanner::ok, error_t> {
        std::vector<Diagnostic> diagnostics{};
        std::vector<scanning::template_t> templates{};
        std::vector<std::string> extra_declarations{};
        for(const auto& filepath : filepaths) {
            for(const auto &entry: std::filesystem::directory_iterator(filepath)) {
                try {
                    if(should_ignore(entry, ignore_list)) {
                        spdlog::trace("ignoring file {0}", entry.path().c_str());
                        continue;
                    }

                    std::ifstream input_filestream(entry.path());
                    auto json_file = nlohmann::json::parse(input_filestream,
                            /* callback */ nullptr,
                            /* allow exceptions */ true,
                            /* ignore_comments */ true);
                    if(json_file.contains("name")) {
                        spdlog::trace("loading file {0}", entry.path().c_str());
                        templates.push_back(scan_template(entry.path(), json_file));
                    } else if(json_file.contains("parts")) {
                        spdlog::trace("loading parts file {0}", entry.path().c_str());
                        extra_declarations.push_back(scan_parts(json_file));
                    } else
                        spdlog::trace("ignoring file {0} (not a valid model file)", entry.path().c_str());
                } catch (std::exception& e) {
                    spdlog::error("unable to parse json file {0}: {1}", entry.path().c_str(), e.what());
                    diagnostics.push_back(ctx.get_diagnostic_factory().without_context().create_diagnostic(json_error(entry.path(), e.what())));
                }
            }
        }

        if(!extra_declarations.empty()) {
            auto main = std::find_if(templates.begin(), templates.end(), [](const scanning::template_t& t) {
                    return std::find_if(t.modifiers.begin(), t.modifiers.end(), [](const std::string& m) {
                            return m == "main";
                    }) != t.modifiers.end();
                });
            if(main != templates.end())
                main->declarations.insert(main->declarations.end(), extra_declarations.begin(), extra_declarations.end());
        }
        return scanner::ok{
            .templates=templates,
            .diagnostics=diagnostics
        };
    }

    auto scan_vertex(const nlohmann::json& t) -> scanning::vertex {
        std::string type{};
        std::vector<std::string> modifiers{};
        std::string tt = lower_case(t["type"]);
        if(tt == "normal")
            type = "location";
        if(tt == "initial") {
            type = "location";
            modifiers.push_back("initial");
        }
        if(tt == "final") {
            type = "location";
            modifiers.push_back("final");
        }
        return scanning::vertex{
            .identifer=t["id"],
            .type=type,
            .modifiers=modifiers,
            .debug={
                .name=t["nickname"],
                .position=position{
                    .x=t["x"],
                    .y=t["y"]
                }
            }
        };
    }

    auto scan_edge(const nlohmann::json& t) -> scanning::edge {
        std::optional<std::string> guard{};
        if(t["guard"] != "")
            guard = t["guard"];
        std::optional<std::string> update{};
        if(t["update"] != "")
            guard = t["update"];
        return scanning::edge{
            .identifier=t["uuid"],
            .source=t["source_location"],
            .guard=guard,
            .update=update,
            .target=t["target_location"],
            .debug={}
        };
    }

    auto huppaal_scanner::scan_template(const std::string& filepath, const nlohmann::json& t) const -> scanning::template_t {
        std::vector<scanning::vertex> vertices{};
        for(auto& v : t["vertices"])
            vertices.push_back(scan_vertex(v));
        vertices.push_back(scan_vertex(t["initial_location"]));
        vertices.push_back(scan_vertex(t["final_location"]));

        std::vector<scanning::edge> edges{};
        for(auto& e : t["edges"])
            edges.push_back(scan_edge(e));

        std::vector<std::string> declarations{};
        declarations.push_back(t["declarations"]);

        std::vector<std::string> modifiers{};
        if(t["main"].is_boolean() && t["main"] == true)
            modifiers.push_back("main");

        return scanning::template_t{
            .identifier=ya::uuid_v4(),
            .signature=t["name"],
            .declarations=declarations,
            .vertices=vertices,
            .edges=edges,
            .modifiers=modifiers,
            .debug={
                .name=t["name"],
                .filepath=filepath
            }
        };
    }

    auto scan_part(const nlohmann::json& p) -> std::string {
        std::string id = p["ID"];
        std::string type = lower_case(p["Type"]);
        if(p.contains("ValueType"))
            type = lower_case(p["ValueType"]);
        nlohmann::json initial_value{};
        if(type == "timer")
            initial_value = 0;
        if(p.contains("Value"))
            initial_value = p["Value"];

        // TODO: consider adding an "external/extern" keyword to expr.
        std::stringstream ss{};
        ss << "public " << type << " " << id << " := " << initial_value << " ;";
        return ss.str();
    }

    auto huppaal_scanner::scan_parts(const nlohmann::json& t) const -> std::string {
        std::stringstream ss{};
        for(auto& p : t["parts"])
            ss << scan_part(p) << "\n";
        return ss.str();
    }

    auto huppaal_scanner::should_ignore(const std::filesystem::directory_entry& entry, const std::vector<std::string>& ignore_list) const -> bool {
        return std::any_of(ignore_list.begin(), ignore_list.end(),
                           [&entry, this](const std::string& ig){ return should_ignore(entry, ig); });
    }

    auto huppaal_scanner::should_ignore(const std::filesystem::directory_entry& entry, const std::string& ignore_regex) const -> bool {
        return std::regex_match(entry.path().c_str(), std::regex{ignore_regex});
    }
    
    /* =========== */

    auto huppaal_parser::parse(compiler& ctx, const scanner::ok& stream) const noexcept -> std::expected<parser::ok, error_t> {
        return std::unexpected(error_t({ctx.diag(not_implemented_yet())}));
    }

    /* =========== */

    auto huppaal_semantic_analyzer::analyze(compiler& ctx, const aaltitoad::hawk::parser::ok& ast) const noexcept -> std::expected<aaltitoad::hawk::parser::ok, error_t> {
        return std::unexpected(error_t({ctx.diag(not_implemented_yet())}));
    }

    /* =========== */

    auto huppaal_generator::generate(compiler& ctx, const aaltitoad::hawk::parser::ok& ast) const noexcept -> std::expected<ntta_t, error_t> {
        return std::unexpected(error_t({ctx.diag(not_implemented_yet())}));
    }

    /* =========== */

    auto parser::parse_files(const std::vector<std::string>& files, const std::vector<std::string>& ignore_patterns) -> plugin::parse_result {
        auto compiler = create_compiler();
        auto compile_result = compiler.compile(files, ignore_patterns);
        if(!compile_result.has_value())
            return std::unexpected(plugin::parse_error{compile_result.error().diagnostics});
        return plugin::parse_ok{compile_result.value().ntta, compile_result.value().diagnostics};
    }

    auto parser::parse_model(const Buffer& buffer) -> plugin::parse_result {
        auto compiler = create_compiler();
        return std::unexpected(plugin::parse_error{std::vector<Diagnostic>{compiler.diag(not_implemented_yet())}});
    }

    auto parser::create_compiler() -> compiler {
        return compiler{_scanner, _parser, _semantic_analyzer, _optimizer, _generator};
    }
}

extern "C" {
    const char* get_plugin_name() {
        return "huppaal_parser";
    }

    const char* get_plugin_version() {
        return "v1.1.0";
    }

    plugin_type get_plugin_type() {
        return plugin_type::parser;
    }

    aaltitoad::plugin::parser* create_parser() {
        return new aaltitoad::hawk::huppaal::parser();
    }
}
