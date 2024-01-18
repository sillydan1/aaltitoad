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
#include <cli/cli_common.h>
#include <csignal>
#include <plugin_system/plugin_system.h>
#include <aaltitoadpch.h>
#include <timer>
#include <nlohmann/json.hpp>
#include "cli/lsp/proto/lsp_server.h"
#include "cli_options.h"

void siginthandler(int param) {
    spdlog::info("exiting ({})", param);
    exit(param);
}

int main(int argc, char** argv) {
    signal(SIGINT, siginthandler);
    auto options = get_options();
    auto cli_arguments = get_arguments(options, argc, argv);
    if(cli_arguments["verbosity"])
        spdlog::set_level(static_cast<spdlog::level::level_enum>(SPDLOG_LEVEL_OFF - cli_arguments["verbosity"].as_integer()));
    if(cli_arguments["version"])
        return print_version();
    if(cli_arguments["help"])
        return print_help(argv[0], options);
    if(!is_required_provided(cli_arguments, options))
        return print_required_args();

    spdlog::trace("welcome to {} v{}", PROJECT_NAME, PROJECT_VER);
    aaltitoad::lsp::proto::LanguageServerImpl{
        cli_arguments["port"].as_integer_or_default(5001),
        PROJECT_VER
    }.start();
    spdlog::trace("shutting down {} v{}", PROJECT_NAME, PROJECT_VER);
    return 0;
}
