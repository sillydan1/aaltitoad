#ifndef AALTITOAD_CLI_OPTIONS_H
#define AALTITOAD_CLI_OPTIONS_H
#include <argvparse.h>

std::vector<option_t> get_options() {
    return {
            {"version",    'V', argument_requirement::NO_ARG,       "Print version and exit"},
            {"verbosity",  'v', argument_requirement::REQUIRE_ARG,  "Set verbosity level (6 for max verbosity)"},
            {"input",      'f', argument_requirement::REQUIRE_ARG,  "(Required) input folder containing diagram files to parse and simulate"},
            {"ignore",     'i', argument_requirement::REQUIRE_ARG,  "Specify a file to ignore (-i file1 -i file2 for multiple files)"},
            {"tocker",     't', argument_requirement::REQUIRE_ARG,  "Specify a tocker by name to instantiate"},
    };
}

bool is_required(const std::string& s) {
    if(s == "input")
        return true;
    return false;
}

bool is_required_provided(std::map<std::string, argument_t>& provided_args, const std::vector<option_t>& options) {
    for(auto& opt : options) {
        if(is_required(opt.long_option) && !provided_args[opt.long_option])
            return false;
    }
    return true;
}

#endif //AALTITOAD_CLI_OPTIONS_H
