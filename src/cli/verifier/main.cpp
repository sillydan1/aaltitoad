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
#include "expr-wrappers/ctl-interpreter.h"
#include "query/query_json_loader.h"
#include "spdlog/common.h"
#include "spdlog/spdlog.h"
#include "verification/pick_strategy.h"
#include <aaltitoadpch.h>
#include <config.h>
#include <ctl_syntax_tree.h>
#include <expr-lang/expr-parser.hpp>
#include <expr-lang/expr-scanner.hpp>
#include <ios>
#include <lyra/lyra.hpp>
#include <magic_enum/magic_enum.hpp>
#include <ntta/interesting_tocker.h>
#include <ntta/tta.h>
#include <plugin_system/plugin_system.h>
#include <timer>
#include <verification/forward_reachability.h>

void trace_log_ntta(const aaltitoad::ntta_t& n);
void disable_warnings(const std::vector<std::string>& disable_warns);

int main(int argc, char** argv) {
    try {
        bool show_help = false;
        int verbosity = SPDLOG_LEVEL_INFO;
        bool show_version = false;
        std::vector<std::string> input = {};
        std::vector<std::string> plugin_dirs = {};
        std::string parser = "huppaal_parser";
        bool list_plugins = false;
        std::vector<std::string> ignore = {};
        std::vector<std::string> query = {};
        std::vector<std::string> query_files = {};
        std::string pick_strategy = "first";
        std::vector<std::string> disabled_warnings = {};
        bool list_warnings = false;
        bool no_warnings = false;
        std::optional<std::string> result_file = {};
        bool output_json = false;
        auto cli = lyra::cli()
            | lyra::help(show_help).description("An extendable verifier for Networks of Tick Tock Automata (NTTA)")
                ("show this message")
            | lyra::opt(verbosity, "0-6")
                ["-v"]["--verbosity"]("set verbosity level, default: " + std::to_string(verbosity))
            | lyra::opt(show_version)
                ["-V"]["--version"]("show version")
            | lyra::opt(input, "DIR")
                ["-f"]["--input"]("add input directory to parse and verify").required()
            | lyra::opt(plugin_dirs, "DIR")
                ["-d"]["--plugin-dir"]("use a directory to look for plugins")
            | lyra::opt(parser, "PARSER")
                ["-p"]["--parser"]("set a parser to use, default: " + parser)
            | lyra::opt(list_plugins)
                ["-L"]["--list-plugins"]("list found plugins and exit")
            | lyra::opt(ignore, "GLOB")
                ["-i"]["--ignore"]("add a glob file pattern to ignore")
            | lyra::opt(query, "CTL")
                ["-Q"]["--query"]("add a CTL query to check verify")
            | lyra::opt(query_files, "FILE")
                ["-q"]["--query-file"]("add a json file with CTL queries to check verify")
            | lyra::opt(pick_strategy, "STRATEGY").choices("first", "last", "random")
                ["-s"]["--pick-strategy"]("waiting list pick strategy, default: " + pick_strategy)
            | lyra::opt(disabled_warnings, "WARN")
                ["-w"]["--disable-warning"]("disable a warning")
            | lyra::opt(list_warnings)
                ["-W"]["--list-warnings"]("list all available warnings and exit")
            | lyra::opt(no_warnings)
                ["-m"]["--no-warnings"]("disable all warnings")
            | lyra::opt(result_file, "FILE")
                ["-t"]["--result-file"]("set file to output results to")
            | lyra::opt(output_json)
                ["-j"]["--result-json"]("output results in json format")
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

        auto available_plugins = aaltitoad::plugins::load(plugin_dirs);
        if(list_plugins) {
            std::cout << "available plugins:\n" << available_plugins;
            return 0;
        }

        if(!available_plugins.contains(parser) || available_plugins.at(parser).type != plugin_type::parser) {
            spdlog::critical("no such parser available: '{}'", parser);
            return 1;
        }

        spdlog::debug("parsing with {0} plugin", parser);
        auto p = std::get<parser_ctor_t>(available_plugins.at(parser).function)();
        ya::timer<int> t{};
        auto parse_result = p->parse_files(input, ignore);
        aaltitoad::warnings::print_warnings(parse_result);
        if(!parse_result.has_value()) {
            spdlog::error("compilation failed");
            return 1;
        }

        auto n = parse_result.value().ntta;
        trace_log_ntta(n);
        spdlog::debug("model parsing took {0}ms", t.milliseconds_elapsed());

        t.start();
        std::vector<ctl::syntax_tree_t> queries{};
        aaltitoad::ctl_interpreter ctl_compiler{n.symbols, n.external_symbols};
        for(auto& q : query) {
            spdlog::trace("compiling query '{0}'", q);
            auto qq = ctl_compiler.compile(q);
            std::stringstream ss{}; ss << qq;
            spdlog::trace("resulting tree: {0}", ss.str());
            queries.emplace_back(qq);
        }
        for(auto& f : query_files) {
            spdlog::trace("loading queries in file {0}", f);
            auto json_queries = aaltitoad::load_query_json_file(f, {n.symbols, n.external_symbols});
            queries.insert(queries.end(), json_queries.begin(), json_queries.end());
        }
        spdlog::debug("query parsing took {0}ms", t.milliseconds_elapsed());

        auto strategy = magic_enum::enum_cast<aaltitoad::pick_strategy>(pick_strategy).value_or(aaltitoad::pick_strategy::first);
        spdlog::debug("using pick strategy '{0}'", magic_enum::enum_name(strategy));

        n.add_tocker(std::make_unique<aaltitoad::interesting_tocker>());
        spdlog::trace("starting reachability search for {0} queries", queries.size());
        t.start();
        aaltitoad::forward_reachability_searcher frs{strategy};
        auto results = frs.is_reachable(n, queries);
        spdlog::info("reachability search took {0}ms", t.milliseconds_elapsed());

        // open the results file (std::cout by default)
        spdlog::trace("opening results file stream");
        auto* trace_stream = &std::cout;
        if(result_file.has_value())
            trace_stream = new std::ofstream{result_file.value(), std::ios::app};

        // write json to results file or non-json if not provided
        if(output_json) {
            spdlog::trace("gathering results json data");
            auto json_results = "[]"_json;
            for(auto& result : results) {
                nlohmann::json res{};
                std::stringstream ss{}; ss << result.query;
                res["query"] = ss.str();
                if(result.solution.has_value())
                    res["trace"] = to_json(result.solution.value());
                json_results.push_back(res);
            }
            *trace_stream << json_results << std::endl;
        } else {
            spdlog::trace("printing resuls data (non-json)");
            for(auto& result : results) {
                *trace_stream << result.query << ": " << std::boolalpha << result.solution.has_value();
                if(result.solution.has_value())
                    *trace_stream << result.solution.value();
            }
        }

        return 0;
    } catch (std::exception& any) {
        spdlog::error(any.what());
        std::cout.flush();
        std::cerr.flush();
        return 1;
    }
}

void trace_log_ntta(const aaltitoad::ntta_t& n) {
    if(spdlog::get_level() >= spdlog::level::trace) {
        std::stringstream internal_symbols_ss{};
        internal_symbols_ss << n.symbols;
        spdlog::trace("internal symbols: \n{0}", internal_symbols_ss.str());

        std::stringstream external_symbols_ss{};
        external_symbols_ss << n.external_symbols;
        spdlog::trace("external symbols: \n{0}", external_symbols_ss.str());
        for(auto& c : n.components) {
            spdlog::trace("<instance> '{0}': (initial: '{1}')", c.first, c.second.initial_location);
            std::stringstream nodes_ss{};
            nodes_ss << "nodes: \n";
            for(auto& node : c.second.graph->nodes)
                nodes_ss << node.first << ": " << node.second.data.identifier << "\n";
            spdlog::trace(nodes_ss.str());
            
            std::stringstream edges_ss{};
            edges_ss << "edges: \n";
            for(auto& edge : c.second.graph->edges)
                edges_ss << edge.first.identifier << ": " << 
                    edge.second.source->second.data.identifier << 
                    " -> " <<
                    edge.second.target->second.data.identifier <<
                    " ( " << 
                    edge.second.data.guard << 
                    " )  [ " <<
                    edge.second.data.updates << 
                    " ] \n";
            spdlog::trace(edges_ss.str());
            spdlog::trace("</instance>");
        }
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
