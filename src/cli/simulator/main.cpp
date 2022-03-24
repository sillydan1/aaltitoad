#include <aaltitoadpch.h>
#include <config.h>
#include <parser/h-uppaal-parser.h>
#include "cli_options.h"
#include <Timer.hpp>
#include <tockers/pipe_tocker.h>
#include <extensions/string_extensions.h>

void parse_and_execute_simulator(std::map<std::string, argument_t>& cli_arguments);

int main(int argc, char** argv) {
    auto options = get_options();
    auto cli_arguments = get_arguments(options, argc, argv);
    if(cli_arguments["help"] || !is_required_provided(cli_arguments, options)) {
        std::cout << get_license() << std::endl;
        std::cout << "USAGE: " << argv[0] << " -i /path/to/tta/dir [OPTIONS] \n" << std::endl;
        std::cout << "OPTIONS: " << std::endl;
        print_argument_help(options);
        return 0;
    }
    if(cli_arguments["version"]) {
        std::cout << PROJECT_NAME << " v" << PROJECT_VER << std::endl;
        return 0;
    }
    if(cli_arguments["verbosity"])
        spdlog::set_level(static_cast<spdlog::level::level_enum>(SPDLOG_LEVEL_OFF - cli_arguments["verbosity"].as_integer()));

    parse_and_execute_simulator(cli_arguments);
    return 0;
}

void parse_and_execute_simulator(std::map<std::string, argument_t>& cli_arguments) {
    /// Parser related arguments
    Timer<unsigned int> t{};
    std::__1::vector<std::string> ignore_list{};
    if(cli_arguments["ignore"])
        ignore_list = cli_arguments["ignore"].as_list();

    /// Parse provided model
    t.start();
    auto automata = h_uppaal_parser_t::parse_folder(cli_arguments["input"].as_string(), ignore_list);
    spdlog::info("model parsing took {0}ms", t.milliseconds_elapsed());

    /// Inject tockers
    if(cli_arguments["tocker"]) {
        for(auto& arg : cli_arguments["tocker"].as_list()) {
            if(contains(arg, "pipe")) {
                /// Format: "pipe(/path/to/fileA;/path/to/fileB)"
                spdlog::debug("Instantiating pipe_tocker_t ");
                automata.tockers.emplace_back(std::__1::make_unique<pipe_tocker_t>(nullptr, automata));
                continue;
            }
            spdlog::warn("tocker type '{0}' not recognized", arg);
        }
    }

    /// Run
    t.start();
    auto x = 10;
    for(int i = 0; i < x; i++) {
        automata.tick();
        automata.tock();
    }
    spdlog::info("{1} ticks took {0}ms", t.milliseconds_elapsed(), x);
}
