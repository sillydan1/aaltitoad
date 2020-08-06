/**
 * Copyright (C) 2020 Asger Gitz-Johansen

   This file is part of mave.

    mave is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    mave is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with mave.  If not, see <https://www.gnu.org/licenses/>.
 */
#include "CLIConfig.h"
#include <algorithm>
#include <iostream>

CLIConfig::CLIConfig() {
    /// Add/Remove Command Line options here.
    cliOptions = {
            { option_requirement::REQUIRE,
              {"input",  'i', argument_requirement::REQUIRE_ARG ,
                "Input file"} },
            { option_requirement::REQUIRE,
              {"output", 'o', argument_requirement::REQUIRE_ARG,
                "Output file. Will be created, if not already exists"} },
            { option_requirement::REQUIRE,
              {"in-type",'n', argument_requirement::REQUIRE_ARG,
                "The type of input modelling language"} },
            { option_requirement::REQUIRE,
              {"out-type",'u', argument_requirement::REQUIRE_ARG,
                "The type of output modelling language"} }
    };
    status_code = EXIT_SUCCESS;
}

void CLIConfig::ParseCLIOptionsAndCheckForRequirements(int argc, char** argv) {
    auto cliOpts = GetCLIOptionsOnly();
    providedOptions = get_arguments(cliOpts, argc, argv);
    EnsureRequiredOptionsAreProvided();
}

std::vector<option_t> CLIConfig::GetCLIOptionsOnly() {
    std::vector<option_t> output{};
    output.reserve(cliOptions.size());
    std::transform(cliOptions.begin(), cliOptions.end(),
                   std::back_inserter(output),
                   [](std::pair<option_requirement, option_t> element) -> option_t { return element.second; });
    return output;
}

void CLIConfig::EnsureRequiredOptionsAreProvided() {
    auto aRequiredOptionNotThere = std::any_of(cliOptions.begin(), cliOptions.end(),
            [this] (auto el) -> bool
            {
                if(el.first == option_requirement::REQUIRE) {
                    return !providedOptions[el.second.long_option];
                } return false;
            });
    if(aRequiredOptionNotThere)
        status_code = EXIT_FAILURE;
}

void CLIConfig::PrintHelpMessage(const char *const *argv) {
    std::cout << argv[0] << " is a program that translates Modelling languages to other modelling languages."
                            " Below are the possible options.\nCopyright (C) 2020  Asger Gitz-Johansen\n"
                            "\n"
                            "    This program is free software: you can redistribute it and/or modify\n"
                            "    it under the terms of the GNU General Public License as published by\n"
                            "    the Free Software Foundation, either version 3 of the License, or\n"
                            "    (at your option) any later version.\n"
                            "\n"
                            "    This program is distributed in the hope that it will be useful,\n"
                            "    but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
                            "    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
                            "    GNU General Public License for more details.\n"
                            "\n"
                            "    You should have received a copy of the GNU General Public License\n"
                            "    along with this program.  If not, see <https://www.gnu.org/licenses/>.\n";
    print_argument_help(GetCLIOptionsOnly());
}

bool CLIConfig::operator[](const std::string &lookup) {
    return providedOptions[lookup].operator bool();
}
