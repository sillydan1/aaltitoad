#include <aaltitoadpch.h>
#include <config.h>
#include "cli_options.h"
#include "../cli_common.h"
#include <timer>
#include <plugin_system/plugin_system.h>
#include <numeric>
#include <ntta/interesting_tocker.h>

void parse_and_execute_simulator(std::map<std::string, argument_t>& cli_arguments);
auto load_plugins(std::map<std::string, argument_t>& cli_arguments) -> plugin_map_t;
auto instantiate_tocker(const std::string& arg, const plugin_map_t& available_plugins, const aaltitoad::ntta_t& automata) -> std::optional<aaltitoad::tocker_t*>;

int main(int argc, char** argv) {
    auto options = get_options();
    auto cli_arguments = get_arguments(options, argc, argv);
    if(cli_arguments["verbosity"])
        spdlog::set_level(static_cast<spdlog::level::level_enum>(SPDLOG_LEVEL_OFF - cli_arguments["verbosity"].as_integer()));
    if(cli_arguments["version"])
        return print_version();
    if(cli_arguments["list-warn"])
        return list_warnings();
    if(cli_arguments["help"])
        return print_help(argv[0], options);
    if(!is_required_provided(cli_arguments, options))
        return print_required_args();

    spdlog::trace("Welcome to {0} v{1}", PROJECT_NAME, PROJECT_VER);

    if(cli_arguments["no-warn"])
        aaltitoad::warnings::disable_all();
    disable_warnings(cli_arguments["disable-warn"].as_list_or_default({}));

    parse_and_execute_simulator(cli_arguments);
    return 0;
}

void parse_and_execute_simulator(std::map<std::string, argument_t>& cli_arguments) {
    /// Load plugins
    auto available_plugins = load_plugins(cli_arguments);
    if(cli_arguments["list-plugins"]) {
        auto ss = std::stringstream{} << available_plugins;
        spdlog::info("Available plugins:\n{0}", ss.str());
        return;
    }

    /// Parser related arguments
    ya::timer<unsigned int> t{};
    std::vector<std::string> ignore_list{};
    if(cli_arguments["ignore"])
        ignore_list = cli_arguments["ignore"].as_list();

    /// Get the parser
    auto selected_parser = cli_arguments["parser"].as_string_or_default("huppaal_parser");
    if(!available_plugins.contains(selected_parser) || available_plugins.at(selected_parser).type != plugin_type::parser) {
        spdlog::critical("No such parser available: '{0}'", selected_parser);
        return;
    }

    /// Parse provided model
    spdlog::info("Parsing with {0} plugin", selected_parser);
    auto parser = std::get<parser_func_t>(available_plugins.at(selected_parser).function);
    t.start();
    auto automata = std::unique_ptr<aaltitoad::ntta_t>(parser(cli_arguments["input"].as_list(), ignore_list));
    spdlog::info("Model parsing took {0}ms", t.milliseconds_elapsed());

    /// Inject tockers - CLI Format: "name(argument)"
    automata->add_tocker(std::make_shared<aaltitoad::interesting_tocker>());
    // for(auto& arg : cli_arguments["tocker"].as_list_or_default({})) {
    //     auto tocker = instantiate_tocker(arg, available_plugins, *automata);
    //     if(tocker.has_value())
    //         automata->tockers.emplace_back(tocker.value());
    // }

    /// Run
    t.start();
    auto x = cli_arguments["ticks"].as_integer_or_default(-1);
    spdlog::info("Simulating...");
    unsigned int i = 0;
#ifdef NDEBUG
    try {
#endif
        for (; i < x || x < 0; i++) {
            if(spdlog::get_level() <= spdlog::level::trace) {
                std::stringstream ss{};
                ss << "state:\n" << *automata;
                spdlog::trace(ss.str());
            }
            auto tock_changes = automata->tock();
            if(!tock_changes.empty())
                automata->apply(tock_changes[0]);
            auto tick_changes = automata->tick();
            if(!tick_changes.empty())
                automata->apply(tick_changes[0]);
        }
#ifdef NDEBUG
    } catch (std::exception& e) {
        spdlog::error(e.what());
    }
#endif
    spdlog::info("{0} ticks took {1}ms", i, t.milliseconds_elapsed());
}

auto load_plugins(std::map<std::string, argument_t>& cli_arguments) -> plugin_map_t {
    auto rpath = std::getenv("AALTITOAD_LIBPATH");
    std::vector<std::string> look_dirs = { ".", "lib", "plugins" };
    if(rpath)
        look_dirs.emplace_back(rpath);
    auto provided_dirs = cli_arguments["plugin-dir"].as_list_or_default({});
    look_dirs.insert(look_dirs.end(), provided_dirs.begin(), provided_dirs.end());
    return aaltitoad::plugins::load(look_dirs);
}

auto instantiate_tocker(const std::string& arg, const plugin_map_t& available_plugins, const aaltitoad::ntta_t& automata) -> std::optional<aaltitoad::tocker_t*> {
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
        if(available_plugins.at(s[0]).type != plugin_type::tocker) {
            spdlog::error("{0} is not a tocker plugin", s[0]);
            return {};
        }
        auto tocker_ctor = std::get<tocker_ctor_t>(available_plugins.at(s[0]).function);
        return tocker_ctor(s[1].substr(0, s[1].size() - 1), automata);
    } catch (std::exception& e) {
        spdlog::error("Error during tocker instantiation: {0}", e.what());
        return {};
    }
}
