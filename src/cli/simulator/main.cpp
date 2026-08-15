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
#include "util/warnings.h"
#include <aaltitoadpch.h>
#include <config.h>
#include <lyra/lyra.hpp>
#include <magic_enum/magic_enum.hpp>
#include <ntta/interesting_tocker.h>
#include <plugin_system/plugin_system.h>
#include <timer>

auto instantiate_tocker(const std::string& arg, const plugin_map_t& available_plugins, const aaltitoad::ntta_t& automata) -> std::optional<std::unique_ptr<aaltitoad::tocker_t>>;
void disable_warnings(const std::vector<std::string>& disable_warns);

int main(int argc, char** argv) {
    bool show_help = false;
    int verbosity = SPDLOG_LEVEL_INFO;
    bool show_version = false;
    std::vector<std::string> input = {};
    std::vector<std::string> plugin_dirs = {};
    std::string parser = "huppaal_parser";
    bool list_plugins = false;
    std::vector<std::string> ignore = {};
    std::vector<std::string> tockers = {};
    int ticks = -1;
    std::vector<std::string> disabled_warnings = {};
    bool list_warnings = false;
    bool no_warnings = false;
    auto cli = lyra::cli()
        | lyra::help(show_help).description("NTTA simulator / runtime with possibility of extensions through tockers")
            ("show this message")
        | lyra::opt(verbosity, "0-6")
            ["-v"]["--verbosity"]("set verbosity level, default: " + std::to_string(verbosity))
        | lyra::opt(show_version)
            ["-V"]["--version"]("show version")
        | lyra::opt(input, "DIR")
            ["-f"]["--input"]("add input directory to parse and simulate").required()
        | lyra::opt(plugin_dirs, "DIR")
            ["-d"]["--plugin-dir"]("use a directory to look for plugins")
        | lyra::opt(parser, "PARSER")
            ["-p"]["--parser"]("set a parser to use, default: " + parser)
        | lyra::opt(list_plugins)
            ["-L"]["--list-plugins"]("list found plugins and exit")
        | lyra::opt(ignore, "GLOB")
            ["-i"]["--ignore"]("add a glob file pattern to ignore")
        | lyra::opt(tockers, "TOCKER")
            ["-t"]["--tocker"]("add a tocker to instantiate")
        | lyra::opt(ticks, "NUM")
            ["-n"]["--ticks"]("set amount of ticks to run, default: " + std::to_string(ticks))
        | lyra::opt(disabled_warnings, "WARN")
            ["-w"]["--disable-warning"]("disable a warning")
        | lyra::opt(list_warnings)
            ["-W"]["--list-warnings"]("list all available warnings and exit")
        | lyra::opt(no_warnings)
            ["-m"]["--no-warnings"]("disable all warnings")
        ;
    auto args = cli.parse({argc, argv});
    if(show_help) {
        std::cout << cli << std::endl;
        return 0;
    }
    if(verbosity < 0 || verbosity > SPDLOG_LEVEL_OFF) {
        std::cout << "verbosity must be within 0-6" << std::endl;
        return 1;
    }
    spdlog::set_level(static_cast<spdlog::level::level_enum>(SPDLOG_LEVEL_OFF - verbosity));
    spdlog::trace("welcome to {} v{}", PROJECT_NAME, PROJECT_VER);
    if(show_version) {
        std::cout << PROJECT_NAME << " v" << PROJECT_VER << std::endl;
        return 0;
    }
    if(!args) {
        spdlog::error(args.message());
        return 1;
    }
    if(list_warnings) { 
        std::cout << "[WARNINGS]:\n";
        for(const auto& warning : aaltitoad::warnings::descriptions())
            std::cout << "\t - [" << magic_enum::enum_name(warning.first) << "]: " << warning.second << "\n";
        return 0;
    }
    if(no_warnings)
        aaltitoad::warnings::disable_all();
    disable_warnings(disabled_warnings);

    /// Load plugins
    auto available_plugins = aaltitoad::plugins::load(plugin_dirs);
    if(list_plugins) {
        std::cout << "available plugins:\n" << available_plugins;
        return 0;
    }

    /// Get the parser
    if(!available_plugins.contains(parser) || available_plugins.at(parser).type != plugin_type::parser) {
        spdlog::error("no such parser available: '{0}'", parser);
        return 1;
    }

    /// Parse provided model
    spdlog::trace("parsing with {0} plugin", parser);
    auto p = std::get<parser_ctor_t>(available_plugins.at(parser).function)();
    ya::timer<unsigned int> t{};
    auto parse_result = p->parse_files(input, ignore);
    spdlog::trace("model parsing took {0}ms", t.milliseconds_elapsed());
    aaltitoad::warnings::print_warnings(parse_result);
    if(!parse_result.has_value()) {
        spdlog::error("compilation failed");
        return 1;
    }
    auto automata = std::move(parse_result.value().ntta);

    /// Inject tockers - CLI Format: "name(argument)"
    for(auto& arg : tockers) {
        auto tocker = instantiate_tocker(arg, available_plugins, automata);
        if(tocker.has_value())
            automata.tockers.emplace_back(std::move(tocker.value()));
    }

    /// Run
    t.start();
    spdlog::trace("simulating...");
    unsigned int i = 0;
#ifdef NDEBUG
    try {
#endif
        for (; i < ticks || ticks < 0; i++) {
            if(spdlog::get_level() <= spdlog::level::trace) {
                std::stringstream ss{};
                ss << "state:\n" << automata;
                spdlog::trace(ss.str());
            }
            auto tock_changes = automata.tock();
            if(!tock_changes.empty())
                automata.apply(tock_changes[0]);
            auto tick_changes = automata.tick();
            if(!tick_changes.empty())
                automata.apply(tick_changes[0]);
        }
#ifdef NDEBUG
    } catch (std::exception& e) {
        spdlog::critical(e.what());
    }
#endif
    spdlog::trace("{0} ticks took {1}ms", i, t.milliseconds_elapsed());
}

auto instantiate_tocker(const std::string& arg, const plugin_map_t& available_plugins, const aaltitoad::ntta_t& automata) -> std::optional<std::unique_ptr<aaltitoad::tocker_t>> {
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
        return std::unique_ptr<aaltitoad::tocker_t>(tocker_ctor(s[1].substr(0, s[1].size() - 1), automata));
    } catch (std::exception& e) {
        spdlog::error("tocker instantiation failed: {0}", e.what());
        return {};
    }
}

void disable_warnings(const std::vector<std::string>& disable_warns) {
    aaltitoad::warnings::enable_all();
    for(auto& w : disable_warns) {
        auto opt = magic_enum::enum_cast<aaltitoad::w_t>(w);
        if(opt.has_value())
            aaltitoad::warnings::disable_warning(opt.value());
        else
            spdlog::warn("not a warning: {0}", w);
    }
}
