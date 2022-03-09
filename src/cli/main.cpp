/**
 * Copyright (C) 2020 Asger Gitz-Johansen

   This file is part of aaltitoad.

    aaltitoad is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    aaltitoad is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with aaltitoad.  If not, see <https://www.gnu.org/licenses/>.
 */
#include <aaltitoadpch.h>
#include "cli_options.h"
#include "config.h"

int main(int argc, char** argv) {
    argc-=(argc>0); argv+=(argc>0); // skip program name argv[0] if present
    option::Stats  stats(usage, argc, argv);
    option::Option options[stats.options_max];
    option::Option buffer[stats.buffer_max];
    option::Parser parse(usage, argc, argv, options, buffer);
    if (options[option::HELP] || argc == 0) {
        std::cout << PROJECT_NAME << " v" << PROJECT_VER << std::endl;
        option::printUsage(std::cout, usage);
        return 0;
    }
    if(options[option::VERSION]) {
        std::cout << PROJECT_NAME << " v" << PROJECT_VER << std::endl;
        return 0;
    }

    auto v = options[option::VERBOSITY_SHORT].count();
    if(options[option::VERBOSITY])
        v = std::max(v, std::stoi(options[option::VERBOSITY].arg));
    spdlog::set_level(static_cast<spdlog::level::level_enum>(SPDLOG_LEVEL_OFF - v));
    spdlog::trace("trace");
    spdlog::debug("debug");
    spdlog::info("info");
    spdlog::warn("warn");
    spdlog::error("error");
    spdlog::critical("critical");

    if(options[option::INPUT_FILEPATH] && options[option::INPUT_FILEPATH].arg)
        std::cout << "Input File " << options[option::INPUT_FILEPATH].arg << std::endl;

    for(option::Option* opt = options[option::UNKNOWN]; opt; opt = opt->next())
        std::cout << "Unknown option: " << opt->name << " ignoring...\n";

    return 0;
}
