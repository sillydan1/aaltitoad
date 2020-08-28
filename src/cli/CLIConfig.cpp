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

CLIConfig::CLIConfig() {
    /// Add/Remove Command Line options here.
    cliOptions = {
            { option_requirement::REQUIRE,
              {"input",  'i', argument_requirement::REQUIRE_ARG ,
                "[DIR] Input directory"} },
            { option_requirement::OPTIONAL,
              {"output", 'o', argument_requirement::REQUIRE_ARG,
                "[DIR]/[FILENAME] Output file. Will be created, if not already exists"} },
            { option_requirement::OPTIONAL,
              {"verbosity",'v', argument_requirement::REQUIRE_ARG,
                 "[0-6] The level of verbosity. Default is 5 (6 is SILENT/OFF)"} }
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

bool CLIConfig::isElementRequiredAndMissing(const std::pair<option_requirement, option_t>& el) {
    return el.first == option_requirement::REQUIRE && !providedOptions[el.second.long_option];
}

void CLIConfig::EnsureRequiredOptionsAreProvided() {
    for(auto& el : cliOptions) {
        if(isElementRequiredAndMissing(el)) {
            status_code = EXIT_FAILURE;
            return;
        }
    }
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
                            "    along with this program.  If not, see <https://www.gnu.org/licenses/>.\n\n"
                            "USAGE: " << argv[0] << " -i /path/to/project/dir/ [OPTIONS]\n\n"
                            "OPTIONS:\n";
    print_argument_help(GetCLIOptionsOnly());
}

argument_t CLIConfig::operator[](const std::string &lookup) {
    return providedOptions[lookup];
}
