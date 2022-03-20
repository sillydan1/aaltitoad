#include <aaltitoadpch.h>
#include <config.h>
#include <parser/h-uppaal-parser.h>
#include "cli_options.h"
#include <Timer.hpp>

void print_state(const ntta_t& automata) {
    std::stringstream ss{};
    ss << stream_mods::json << automata;
    spdlog::info(ss.str());
}

int main(int argc, char** argv) {
    Timer<unsigned int> t{};
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

    std::vector<std::string> ignore_list{};
    if(cli_arguments["ignore"])
        ignore_list = cli_arguments["ignore"].as_list();

    spdlog::info("cli parsing took {0}ms", t.milliseconds_elapsed());
    t.start();
    auto automata = h_uppaal_parser_t::parse_folder(cli_arguments["input"].as_string(), ignore_list);

    spdlog::info("model parsing took {0}ms", t.milliseconds_elapsed());
    t.start();
    auto x = 10;
    for(int i = 0; i < x; i++) {
        automata.tick();
        print_state(automata);
    }
    spdlog::info("{1} ticks took {0}ms", t.milliseconds_elapsed(), x);
    return 0;
}
