#include <aaltitoadpch.h>
#include <config.h>
#include <parser/h-uppaal-parser.h>
#include "cli_options.h"
#include <Timer.hpp>
#include <plugin_system/tocker_plugin_system.h>
#include <extensions/string_extensions.h>

void parse_and_execute_simulator(std::map<std::string, argument_t>& cli_arguments);
tocker_map_t load_tockers(std::map<std::string, argument_t>& cli_arguments);
auto instantiate_tocker(const std::string& arg, const tocker_map_t& available_tockers, const ntta_t& automata) -> std::optional<tocker_t*>;

int main(int argc, char** argv) {
    auto options = get_options();
    auto cli_arguments = get_arguments(options, argc, argv);
    if(cli_arguments["verbosity"])
        spdlog::set_level(static_cast<spdlog::level::level_enum>(SPDLOG_LEVEL_OFF - cli_arguments["verbosity"].as_integer()));
    if(cli_arguments["help"] || !is_required_provided(cli_arguments, options)) {
        std::cout << get_license() << std::endl;
        std::cout << PROJECT_NAME << " v" << PROJECT_VER << std::endl;
        std::cout << "USAGE: " << argv[0] << " -i /path/to/tta/dir [OPTIONS] \n" << std::endl;
        std::cout << "OPTIONS: " << std::endl;
        print_argument_help(options);
        return 0;
    }
    if(cli_arguments["version"]) {
        std::cout << PROJECT_NAME << " v" << PROJECT_VER << std::endl;
        return 0;
    }

    parse_and_execute_simulator(cli_arguments);
    return 0;
}

void parse_and_execute_simulator(std::map<std::string, argument_t>& cli_arguments) {
    /// Load tockers
    auto available_tockers = load_tockers(cli_arguments);
    if(cli_arguments["list-tockers"]) {
        std::cout << "Available Tockers: " << std::endl;
        for(auto& t : available_tockers)
            std::cout << "  - " << t.first << std::endl;
        return;
    }

    /// Parser related arguments
    Timer<unsigned int> t{};
    std::vector<std::string> ignore_list{};
    if(cli_arguments["ignore"])
        ignore_list = cli_arguments["ignore"].as_list();

    /// Parse provided model
    t.start();
    auto automata = h_uppaal_parser_t::parse_folder(cli_arguments["input"].as_string(), ignore_list);
    spdlog::info("model parsing took {0}ms", t.milliseconds_elapsed());

    /// Inject tockers - CLI Format: "name(argument)"
    for(auto& arg : cli_arguments["tocker"].as_list_or_default({})) {
        auto tocker = instantiate_tocker(arg, available_tockers, automata);
        if(tocker.has_value())
            automata.tockers.emplace_back(tocker.value());
    }

    /// Run
    t.start();
    auto x = 10;
    for(int i = 0; i < x; i++) {
        automata.tock();
        automata.tick();
    }
    spdlog::info("{0} ticks took {1}ms", x, t.milliseconds_elapsed());
}

tocker_map_t load_tockers(std::map<std::string, argument_t>& cli_arguments) {
    // TODO: Figure out what are the most common env vars for library paths (No, not $PATH - that is for executables)
    auto rpath = std::getenv("RPATH");
    std::vector<std::string> look_dirs = { "." };
    if(rpath)
        look_dirs.emplace_back(rpath);
    if(cli_arguments["tocker-dir"]) {
        auto elements = cli_arguments["tocker-dir"].as_list();
        look_dirs.insert(look_dirs.end(), elements.begin(), elements.end());
    }
    return tocker_plugin_system::load(look_dirs);
}

auto instantiate_tocker(const std::string& arg, const tocker_map_t& available_tockers, const ntta_t& automata) -> std::optional<tocker_t*> {
    try {
        auto s = split(arg, "(");
        if(s.size() < 2) {
            spdlog::error("Invalid tocker instantiation format. It should be 'tocker(<argument>)'");
            return {};
        }
        if(available_tockers.find(s[0]) == available_tockers.end()) {
            spdlog::warn("tocker type '{0}' not recognized", arg);
            return {};
        }
        return available_tockers.at(s[0])(s[1].substr(0, s[1].size() - 1), automata);
    } catch (std::exception& e) {
        spdlog::error("Error during tocker instantiation: {0}", e.what());
        return {};
    }
}
