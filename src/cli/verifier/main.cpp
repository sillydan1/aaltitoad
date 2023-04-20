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
#include <aaltitoadpch.h>
#include <ctl_syntax_tree.h>
#include <ntta/tta.h>
#include <timer>
#include <plugin_system/plugin_system.h>
#include <verification/forward_reachability.h>
#include <ntta/interesting_tocker.h>
#include "cli_options.h"
#include "../cli_common.h"
#include <expr-lang/expr-scanner.hpp>
#include <expr-lang/expr-parser.hpp>
#include "expr-wrappers/ctl-interpreter.h"
#include "magic_enum.hpp"
#include "query/query_json_loader.h"
#include "spdlog/common.h"
#include "spdlog/spdlog.h"
#include "verification/pick_strategy.h"

auto load_plugins(std::map<std::string, argument_t>& cli_arguments) -> plugin_map_t;
void trace_log_ntta(const aaltitoad::ntta_t& n);

int main(int argc, char** argv) {
    try {
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
        spdlog::trace("welcome to {0} v{1}", PROJECT_NAME, PROJECT_VER);
        if(cli_arguments["no-warn"])
            aaltitoad::warnings::disable_all();
        disable_warnings(cli_arguments["disable-warn"].as_list_or_default({}));

        auto available_plugins = load_plugins(cli_arguments);
        if(cli_arguments["list-plugins"]) {
            std::cout << "available plugins:\n" << available_plugins;
            return 0;
        }

        auto selected_parser = cli_arguments["parser"].as_string_or_default("hawk_parser");
        if(!available_plugins.contains(selected_parser) || available_plugins.at(selected_parser).type != plugin_type::parser) {
            spdlog::critical("no such parser available: '{0}'", selected_parser);
            return 1;
        }

        spdlog::debug("parsing with {0} plugin", selected_parser);
        auto inputs = cli_arguments["input"].as_list();
        auto ignore = cli_arguments["ignore"].as_list_or_default({});
        auto parser = std::get<parser_func_t>(available_plugins.at(selected_parser).function);
        ya::timer<int> t{};
        std::unique_ptr<aaltitoad::ntta_t> n{parser(inputs, ignore)};
        trace_log_ntta(*n);
        spdlog::debug("model parsing took {0}ms", t.milliseconds_elapsed());

        t.start();
        std::vector<ctl::syntax_tree_t> queries{};
        aaltitoad::ctl_interpreter ctl_compiler{n->symbols, n->external_symbols};
        for(auto& q : cli_arguments["query"].as_list_or_default({})) {
            spdlog::trace("compiling query '{0}'", q);
            auto qq = ctl_compiler.compile(q);
            std::stringstream ss{}; ss << qq;
            spdlog::trace("resulting tree: {0}", ss.str());
            queries.emplace_back(qq);
        }
        for(auto& f : cli_arguments["query-file"].as_list_or_default({})) {
            spdlog::trace("loading queries in file {0}", f);
            auto json_queries = aaltitoad::load_query_json_file(f, {n->symbols, n->external_symbols});
            queries.insert(queries.end(), json_queries.begin(), json_queries.end());
        }
        spdlog::debug("query parsing took {0}ms", t.milliseconds_elapsed());

        auto strategy_s = cli_arguments["pick-strategy"].as_string_or_default("first");
        auto strategy = magic_enum::enum_cast<aaltitoad::pick_strategy>(strategy_s).value_or(aaltitoad::pick_strategy::first);
        spdlog::debug("using pick strategy '{0}'", magic_enum::enum_name(strategy));

        n->add_tocker(std::make_unique<aaltitoad::interesting_tocker>());
        spdlog::trace("starting reachability search for {0} queries", queries.size());
        t.start();
        aaltitoad::forward_reachability_searcher frs{strategy};
        auto results = frs.is_reachable(*n, queries);
        spdlog::info("reachability search took {0}ms", t.milliseconds_elapsed());

        // gather and return results
        auto trace_file = cli_arguments["trace-file"].as_string_or_default("");
        auto* trace_stream = &std::cout;
        if(trace_file != "")
            trace_stream = new std::ofstream{trace_file, std::ios::app};
        for(auto& result : results) {
            *trace_stream << result.query << ": " << std::boolalpha << result.solution.has_value() << "\n";
            if(result.solution.has_value())
                *trace_stream << result.solution.value(); // TODO: This should be json formatted
        }

        return 0;
    } catch (std::exception& any) {
        spdlog::error(any.what());
        std::cout.flush(); // Flush the output streams, just to be nice
        std::cerr.flush();
        return 1;
    }
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

