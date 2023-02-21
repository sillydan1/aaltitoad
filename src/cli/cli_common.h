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
#ifndef AALTITOAD_CLI_COMMON_H
#define AALTITOAD_CLI_COMMON_H
#include <vector>
#include <argvparse.h>
#include <iostream>
#include <config.h>
#include <magic_enum.hpp>
#include <util/warnings.h>

int print_required_args() {
    std::cout << "Required arguments:\n";
    std::cout << " --input / -f\n";
    return 1;
}

int print_help(const std::string& program_name, const std::vector<option_t>& options) {
    std::cout << get_license() << std::endl;
    std::cout << PROJECT_NAME << " v" << PROJECT_VER << std::endl;
    std::cout << "USAGE: " << program_name << " -f /path/to/tta/dir [OPTIONS] \n" << std::endl;
    std::cout << "OPTIONS: " << std::endl;
    print_argument_help(options);
    return 0;
}

int print_version() {
    std::cout << PROJECT_NAME << " v" << PROJECT_VER << std::endl;
    return 0;
}

void disable_warnings(const std::vector<std::string>& disable_warns) {
    aaltitoad::warnings::enable_all();
    for(auto& w : disable_warns) {
        auto opt = magic_enum::enum_cast<aaltitoad::w_t>(w);
        if(opt.has_value())
            aaltitoad::warnings::disable_warning(opt.value());
        else
            spdlog::warn("not a warning: {0}", w);
    }
}

int list_warnings() {
    std::cout << "[WARNINGS]:\n";
    for(const auto& warning : aaltitoad::warnings::descriptions())
        std::cout << "\t - [" << magic_enum::enum_name(warning.first) << "]: " << warning.second << "\n";
    return 0;
}

#endif //AALTITOAD_CLI_COMMON_H
