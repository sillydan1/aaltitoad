#ifndef AALTITOAD_CLI_OPTIONS_H
#define AALTITOAD_CLI_OPTIONS_H
#include <argvparse.h>


std::vector<option_t> get_options() {
    return {
            //   long name   |short| argument requirement             | Help description
            {"version",    'v', argument_requirement::NO_ARG,       "Print version and exit"},
            {"verbosity",  'V', argument_requirement::REQUIRE_ARG,  "Set verbosity level (6 for max verbosity)"},
            {"input",      'f', argument_requirement::REQUIRE_ARG,  ""}
    };
}



const option::Descriptor usage[] = {
        {option::UNKNOWN, 0, "", "", option::Arg::None, "USAGE: aaltitoad [options]\n\n"
                                                        "Options:" },
        {option::HELP,     0,"h", "help",option::Arg::None, "  --help, -h  \tPrint usage and exit." },
        {option::VERSION,0,"V", "version",option::Arg::None, "  --version, -V  \tPrint version and exit" },
        {option::VERBOSITY,0,"", "verbosity",option::Arg::Optional, "  --verbosity, -v  \tSet verbosity level (--verbosity=6 or -vvvvvv for max verbosity)" },
        {option::VERBOSITY_SHORT,0,"v", "",option::Arg::None },
        {option::INPUT_FILEPATH,0,"i", "input",option::Arg::Optional, "  --input, -i      \t(Required) Set input filepath. Example: --input=path/to/model or -i path/to/model" },
        {0,0,0,0,0,0}
};

#endif //AALTITOAD_CLI_OPTIONS_H
