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
#include <cli/CLIConfig.h>

CLIConfig::CLIConfig() {
    /// Add/Remove Command Line options here.
    cliOptions = {
            { option_requirement::Require,
              {"input",  'i', argument_requirement::REQUIRE_ARG ,
                "[DIR] Input directory"} },
            { option_requirement::Optional,
              {"output", 'o', argument_requirement::REQUIRE_ARG,
                "[DIR]/[FILENAME] Output file. Will be created, if not already exists"} },
            { option_requirement::Optional,
                    {"immediate-output", '!', argument_requirement::NO_ARG,
                            "Immediately output a trace once a query has been satisfied"} },
            { option_requirement::Optional,
              {"query", 'q', argument_requirement::REQUIRE_ARG,
                 "[DIR]/[FILENAME] File with queries to be verified. This flag is required for verification"} },
            { option_requirement::Optional,
              {"verbosity",'v', argument_requirement::REQUIRE_ARG,
                 "[0-6] The level of verbosity. (0: OFF | 1: CRITICAL | 2: ERROR | 3: WARN | 4: INFO | 5: DEBUG | 6: TRACE). Default is 2"} },
            { option_requirement::Optional,
            {"version",'e', argument_requirement::NO_ARG,
                "Displays the version number"} },
            { option_requirement::Optional,
            {"nondeterminism-strategy", 'n', argument_requirement::REQUIRE_ARG,
                            "[0-3] Determines which edge to pick when encountering nondeterministic choice. Default is 0 (0: PANIC | 1: PICK_FIRST | 2: PICK_LAST | 3: PICK_RANDOM)"} },
            { option_requirement::Optional,
              {"parser-disable-check", 'd', argument_requirement::NO_ARG,
              "Disables proper formation checks."} },
            /*{ option_requirement::Optional,
                    {"trace", 't', argument_requirement::REQUIRE_ARG,
                            "[1-N] Outputs a trace of the input automata of provided amount of steps. Use with '--trace-output' option"} },*/
            { option_requirement::Optional,
                    {"trace-output", 'u', argument_requirement::REQUIRE_ARG,
                            "[DIR]/[FILENAME] Output file for traces. Use together with '--trace' option"} },
            { option_requirement::Optional,
                    {"timing-info", 'f', argument_requirement::REQUIRE_ARG,
                            "[DIR]/[FILENAME] Input file for timing information/instructions. Use together with '--trace' option"} },
            { option_requirement::Optional,
                    {"ignore-update-influence", 'z', argument_requirement::NO_ARG,
                            "Forces transitions to be taken, no matter if they have an overlapping update influence. NOTE: Does not disable the warning"} },
            { option_requirement::Optional,
                    {"explosion-limit", 'l', argument_requirement::REQUIRE_ARG,
                            "[INTEGER] Sets a limit on what the maximum amount of interesting changes. -1 means no limit. Default is -1. NB! This will result in incorrect answers"} },
            { option_requirement::Optional,
                    {"notrace", 'c', argument_requirement::NO_ARG,
                            "Disable print of traces to stdout"} },
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
    add_help_option(output);
    return output;
}

bool CLIConfig::isElementRequiredAndMissing(const std::pair<option_requirement, option_t>& el) {
    return el.first == option_requirement::Require && !providedOptions[el.second.long_option];
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
    std::cout << argv[0] << " is a program that performs model verification techniques to the provided unfolded TTA."
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
