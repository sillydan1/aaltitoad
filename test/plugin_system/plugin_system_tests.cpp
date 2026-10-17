/**
 * aaltitoad - a verification engine for tick tock automata models
   Copyright (C) 2026 Asger Gitz-Johansen

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
#include "../lsp/temporary_directory.h"
#include <plugin_system/plugin_system.h>
#include <util/warnings.h>
#include <catch2/catch_test_macros.hpp>
#include <spdlog/sinks/ostream_sink.h>
#include <spdlog/spdlog.h>
#include <sstream>

namespace {
    // Collects what is logged while it exists
    struct captured_log_t {
        std::ostringstream text{};
        std::shared_ptr<spdlog::logger> previous{spdlog::default_logger()};

        captured_log_t() {
            auto logger = std::make_shared<spdlog::logger>("captured", std::make_shared<spdlog::sinks::ostream_sink_mt>(text));
            logger->set_level(spdlog::level::trace);
            spdlog::set_default_logger(logger);
        }
        ~captured_log_t() {
            spdlog::set_default_logger(previous);
        }
    };
}

SCENARIO("loading plugins", "[plugin_system]") {
    aaltitoad::warnings::enable_all();
    aaltitoad::test::temporary_directory_t directory{};
    GIVEN("a plugin whose file name does not look like a shared library") {
        std::filesystem::copy_file(AALTITOAD_HAWK_PARSER_PLUGIN, directory.path / "hawk.plugin");
        WHEN("loading the plugins of the directory") {
            auto plugins = aaltitoad::plugins::load({directory.path.string()});
            THEN("the plugin is loaded") {
                REQUIRE(plugins.contains("hawk_parser"));
                REQUIRE(plugins.at("hawk_parser").type == plugin_type::parser);
            }
        }
    }
    GIVEN("a plugin in a directory that is given twice (with different paths)") {
        std::filesystem::copy_file(AALTITOAD_HAWK_PARSER_PLUGIN, directory.path / "libhawk_parser.so");
        WHEN("loading the plugins of the directories") {
            captured_log_t log{};
            auto plugins = aaltitoad::plugins::load({directory.path.string(), (directory.path / "." ).string()});
            THEN("the plugin is loaded once, and nothing is warned about") {
                REQUIRE(plugins.size() == 1);
                REQUIRE(plugins.contains("hawk_parser"));
                INFO(log.text.str());
                REQUIRE(log.text.str().find("[warning]") == std::string::npos);
            }
        }
    }
    GIVEN("files that are not plugins, although their names contain '.so'") {
        directory.write_text("notes.so.txt", "not a shared library");
        std::filesystem::copy_file(AALTITOAD_NON_PLUGIN_LIBRARY, directory.path / "libnot-a-plugin.so");
        WHEN("loading the plugins of the directory") {
            captured_log_t log{};
            auto plugins = aaltitoad::plugins::load({directory.path.string()});
            THEN("nothing is loaded, and nothing is warned about") {
                REQUIRE(plugins.empty());
                INFO(log.text.str());
                REQUIRE(log.text.str().find("[warning]") == std::string::npos);
                REQUIRE(log.text.str().find("not a plugin: '" + (directory.path / "notes.so.txt").string()) != std::string::npos);
                REQUIRE(log.text.str().find("does not define get_plugin_name") != std::string::npos);
            }
        }
    }
}
