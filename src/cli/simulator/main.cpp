#include <aaltitoadpch.h>
#include <config.h>
#include "cli_options.h"
#include <Timer.hpp>
#include <plugin_system/plugin_system.h>

void parse_and_execute_simulator(std::map<std::string, argument_t>& cli_arguments);
auto load_plugins(std::map<std::string, argument_t>& cli_arguments) -> plugin_map_t;
auto instantiate_tocker(const std::string& arg, const plugin_map_t& available_plugins, const ntta_t& automata) -> std::optional<tocker_t*>;

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
    /// Load plugins
    auto available_plugins = load_plugins(cli_arguments);
    if(cli_arguments["list-plugins"] || available_plugins.empty()) {
        std::cout << "Found Plugins: " << std::endl;
        for(auto& t : available_plugins)
            std::cout << "  - " << t.first << " (" << static_cast<unsigned int>(t.second.first) << ")" << std::endl;
        return;
    }

    /// Parser related arguments
    Timer<unsigned int> t{};
    std::vector<std::string> ignore_list{};
    if(cli_arguments["ignore"])
        ignore_list = cli_arguments["ignore"].as_list();

    /// Get the parser
    auto selected_parser = cli_arguments["parser"].as_string_or_default("h_uppaal_parser");
    if(!available_plugins.contains(selected_parser) || available_plugins.at(selected_parser).first != plugin_type::parser) {
        spdlog::critical("No such parser available: '{0}'", selected_parser);
        return;
    }

    /// Parse provided model
    spdlog::debug("Parsing with {0} as a parser", selected_parser);
    auto parser = std::get<parser_func_t>(available_plugins.at(selected_parser).second);
    t.start();
    auto automata = std::unique_ptr<ntta_t>(parser(cli_arguments["input"].as_list(), ignore_list));
    spdlog::info("Model parsing took {0}ms", t.milliseconds_elapsed());

    /// Inject tockers - CLI Format: "name(argument)"
    for(auto& arg : cli_arguments["tocker"].as_list_or_default({})) {
        auto tocker = instantiate_tocker(arg, available_plugins, *automata);
        if(tocker.has_value())
            automata->tockers.emplace_back(tocker.value());
    }

    /// Run
    t.start();
    auto x = cli_arguments["ticks"].as_integer_or_default(-1);
    spdlog::info("Simulating...");
    for(int i = 0; i < x || x < 0; i++) {
        automata->tock();
        automata->tick();
    }
    spdlog::info("{0} ticks took {1}ms", x, t.milliseconds_elapsed());
}

auto load_plugins(std::map<std::string, argument_t>& cli_arguments) -> plugin_map_t {
    // TODO: Figure out what are the most common env vars for library paths (No, not $PATH - that is for executables)
    auto rpath = std::getenv("RPATH");
    std::vector<std::string> look_dirs = { ".", "src/plugins", "plugins" };
    if(rpath)
        look_dirs.emplace_back(rpath);
    auto provided_dirs = cli_arguments["plugin-dir"].as_list_or_default({});
    look_dirs.insert(look_dirs.end(), provided_dirs.begin(), provided_dirs.end());
    return plugins::load(look_dirs);
}

auto instantiate_tocker(const std::string& arg, const plugin_map_t& available_plugins, const ntta_t& automata) -> std::optional<tocker_t*> {
    try {
        auto s = split(arg, "(");
        if(s.size() < 2) {
            spdlog::error("Invalid tocker instantiation format. It should be 'tocker(<argument>)'");
            return {};
        }
        if(available_plugins.find(s[0]) == available_plugins.end()) {
            spdlog::warn("tocker type '{0}' not recognized", arg);
            return {};
        }
        if(available_plugins.at(s[0]).first != plugin_type::tocker) {
            spdlog::error("{0} is not a tocker plugin", s[0]);
            return {};
        }
        auto tocker_ctor = std::get<tocker_ctor_t>(available_plugins.at(s[0]).second);
        return tocker_ctor(s[1].substr(0, s[1].size() - 1), automata);
    } catch (std::exception& e) {
        spdlog::error("Error during tocker instantiation: {0}", e.what());
        return {};
    }
}
