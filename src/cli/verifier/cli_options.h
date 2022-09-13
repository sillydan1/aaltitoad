#ifndef AALTITOAD_CLI_OPTIONS_H
#define AALTITOAD_CLI_OPTIONS_H
#include <vector>
#include <argvparse.h>
#include <iostream>
#include <config.h>
#include <magic_enum.hpp>
#include <warnings.h>

std::vector<option_t> get_options() {
    return {
            {"input",       'f', argument_requirement::REQUIRE_ARG,  "(Required) input folder containing diagram files"},
            {"version",     'V', argument_requirement::NO_ARG,       "Print version and exit"},
            {"verbosity",   'v', argument_requirement::REQUIRE_ARG,  "Set verbosity level (6 for max verbosity)"},
            {"ignore",      'i', argument_requirement::REQUIRE_ARG,  "GNU-style regex for filename(s) to ignore"},

            {"parser",      'p', argument_requirement::REQUIRE_ARG,  "Which parser to use"},
            {"query-file",  'q', argument_requirement::REQUIRE_ARG,  "Query definition json file"},
            {"query",       'Q', argument_requirement::REQUIRE_ARG,  "Add a CTL query to verify"},

            {"plugin-dir",  'P', argument_requirement::REQUIRE_ARG,  "Directories to look for parser plugins"},
            {"list-plugins",'L', argument_requirement::NO_ARG,       "List found plugins and exit"},

            {"disable-warn",'w', argument_requirement::REQUIRE_ARG,  "Disable a warning"},
            {"list-warn",   'W', argument_requirement::NO_ARG,       "List all warnings available"},
            {"no-warn",     'm', argument_requirement::NO_ARG,       "Disable all warnings"},
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
    if(!provided_args["query-file"] && !provided_args["query"]) {
        spdlog::critical("must provide either at least one query with '--query-file' or '--query'");
        return false;
    }
    for(auto& opt : options) {
        if(is_required(opt.long_option) && !provided_args[opt.long_option])
            return false;
    }
    return true;
}

#endif //AALTITOAD_CLI_OPTIONS_H
