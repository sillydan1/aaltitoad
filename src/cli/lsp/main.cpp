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
#include "lsp_server.h"
#include <aaltitoadpch.h>
#include <config.h>
#include <csignal>
#include <lyra/lyra.hpp>
#include <nlohmann/json.hpp>
#include <plugin_system/plugin_system.h>
#include <timer>

void siginthandler(int param) {
    spdlog::info("exiting ({})", param);
    exit(param);
}

int main(int argc, char** argv) {
    signal(SIGINT, siginthandler);
    bool show_help = false;
    int verbosity = SPDLOG_LEVEL_INFO;
    bool show_version = false;
    std::vector<std::string> plugin_dirs = {};
    std::string parser = "huppaal_parser";
    bool list_plugins = false;
    int port = 5001;
    auto cli = lyra::cli()
        | lyra::help(show_help).description("A MLSP (Model Language Server Protocol) server implementation")
            ("show this message")
        | lyra::opt(verbosity, "0-6")
            ["-v"]["--verbosity"]("set verbosity level, default: " + std::to_string(verbosity))
        | lyra::opt(show_version)
            ["-V"]["--version"]("show version")
        | lyra::opt(plugin_dirs, "DIR")
            ["-d"]["--plugin-dir"]("use a directory to look for plugins")
        | lyra::opt(parser, "PARSER")
            ["-p"]["--parser"]("set a parser to use, default: " + parser)
        | lyra::opt(list_plugins)
            ["-L"]["--list-plugins"]("list found plugins and exit")
        | lyra::opt(port, "PORT")
            ["-P"]["--port"]("set port to host the lsp, default: " + std::to_string(port))
        ;
    auto args = cli.parse({argc, argv});
    if(show_help) {
        std::cout << cli << std::endl;
        return 0;
    }
    if(verbosity < 0 || verbosity > SPDLOG_LEVEL_OFF) {
        std::cout << "verbosity must be within 0-6" << std::endl;
        return 1;
    }
    spdlog::set_level(static_cast<spdlog::level::level_enum>(SPDLOG_LEVEL_OFF - verbosity));
    spdlog::trace("welcome to {} v{}", PROJECT_NAME, PROJECT_VER);
    if(show_version) {
        std::cout << PROJECT_NAME << " v" << PROJECT_VER << std::endl;
        return 0;
    }
    if(!args) {
        spdlog::error(args.message());
        return 1;
    }

    spdlog::trace("loading plugins");
    auto available_plugins = aaltitoad::plugins::load(plugin_dirs);
    if(list_plugins) {
        std::cout << "available plugins:\n" << available_plugins;
        return 0;
    }

    spdlog::trace("loading parser");
    if(!available_plugins.contains(parser) || available_plugins.at(parser).type != plugin_type::parser) {
        spdlog::error("no such parser available: '{}'", parser);
        return 1;
    }

    spdlog::trace("building parser {}", parser);
    std::shared_ptr<aaltitoad::plugin::parser> p{std::get<parser_ctor_t>(available_plugins.at(parser).function)()};

    spdlog::trace("starting language server...");
    aaltitoad::lsp::proto::LanguageServerImpl{port, PROJECT_VER, p}.start();

    spdlog::trace("shutting down {} v{}", PROJECT_NAME, PROJECT_VER);
    return 0;
}
