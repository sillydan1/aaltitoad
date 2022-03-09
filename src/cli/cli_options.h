#ifndef AALTITOAD_CLI_OPTIONS_H
#define AALTITOAD_CLI_OPTIONS_H
#include <optionparser.h>

namespace option {
    enum option_t {
        UNKNOWN = 0, HELP, VERSION, VERBOSITY, VERBOSITY_SHORT, INPUT_FILEPATH
    };
}
const option::Descriptor usage[] = {
        {option::UNKNOWN, 0, "", "", option::Arg::None, "USAGE: aaltitoad [options]\n\n"
                                                "Options:" },
        {option::HELP,     0,"h", "help",option::Arg::None, "  --help, -h  \tPrint usage and exit." },
        {option::VERSION,0,"V", "version",option::Arg::None, "  --version, -V  \tPrint version and exit" },
        {option::VERBOSITY,0,"", "verbosity",option::Arg::Optional, "  --verbosity, -v  \tSet verbosity level (--verbosity=4 (or -vvvv) for max verbosity)" },
        {option::VERBOSITY_SHORT,0,"v", "",option::Arg::None },
        {option::INPUT_FILEPATH,0,"i", "input",option::Arg::Optional, "  --input, -i      \tSet input filepath\n"
                                                              "                   \tExample: --input=path/to/model or -ipath/to/model" },
        {0,0,0,0,0,0}
};

#endif //AALTITOAD_CLI_OPTIONS_H
