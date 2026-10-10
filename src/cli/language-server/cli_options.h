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
#ifndef AALTITOAD_LANGUAGE_SERVER_CLI_OPTIONS_H
#define AALTITOAD_LANGUAGE_SERVER_CLI_OPTIONS_H
#include <argvparse.h>

std::vector<option_t> get_options() {
    return {
            {"version",   'V', argument_requirement::NO_ARG,      "Print version and exit"},
            {"verbosity", 'v', argument_requirement::REQUIRE_ARG, "Set verbosity level of the log on stderr (6 for max verbosity). Default is 3 (warnings)"},
    };
}

#endif //AALTITOAD_LANGUAGE_SERVER_CLI_OPTIONS_H
