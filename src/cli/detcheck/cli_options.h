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
#ifndef AALTITOAD_CLI_OPTIONS_H
#define AALTITOAD_CLI_OPTIONS_H

std::vector<option_t> get_options() {
    return {
            {"input",         'f', argument_requirement::REQUIRE_ARG,  "(Required) input folder containing diagram files to parse and simulate"},
            {"version",       'V', argument_requirement::NO_ARG,       "Print version and exit"},
            {"verbosity",     'v', argument_requirement::REQUIRE_ARG,  "Set verbosity level (6 for max verbosity)"},
            {"ignore",        'i', argument_requirement::REQUIRE_ARG,  "Specify a file to ignore (-i file1 -i file2 for multiple files)"},
            {"parser",        'p', argument_requirement::REQUIRE_ARG,  "Specify the parser to use"},
            {"plugin-dir",    'P', argument_requirement::REQUIRE_ARG,  "Specify directories to look for parser plugins"},
            {"instance",      'n', argument_requirement::REQUIRE_ARG,  "Specify tta instances to check"},
            {"instance-file", 'N', argument_requirement::REQUIRE_ARG,  "Specify a json-encoded file containing tta instances to check"},
            {"list-instances",'L', argument_requirement::NO_ARG,       "List available instances and exit"},

            {"known",         'k', argument_requirement::REQUIRE_ARG,  "Specify known symbol declarations"},
            {"known-file",    'K', argument_requirement::REQUIRE_ARG,  "Specify a json-encoded file containing known symbol declarations"},
            {"condition",     'c', argument_requirement::REQUIRE_ARG,  "Specify a condition. This will be added to all bool checks"},
            {"condition-file",'C', argument_requirement::REQUIRE_ARG,  "Specify a json-encoded file containing extra conditions"},
    };
}

bool is_required(const std::string& s) {
    if(s == "input")
        return true;
    return false;
}

bool is_required_provided(std::map<std::string, argument_t>& provided_args, const std::vector<option_t>& options) {
    if(provided_args["version"])
        return true;
    for(auto& opt : options) {
        if(is_required(opt.long_option) && !provided_args[opt.long_option])
            return false;
    }
    return true;
}
#endif //AALTITOAD_CLI_OPTIONS_H
