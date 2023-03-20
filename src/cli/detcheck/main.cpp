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
#include <cli/cli_common.h>
#include <plugin_system/plugin_system.h>
#include <aaltitoadpch.h>
#include <timer>
#include <nlohmann/json.hpp>
#include "cli_options.h"
#include "expr-wrappers/interpreter.h"

auto get_ntta(std::map<std::string, argument_t>& cli_arguments) -> std::unique_ptr<aaltitoad::ntta_t>;
auto load_plugins(std::map<std::string, argument_t>& cli_arguments) -> plugin_map_t;
void find_deadlocks(const std::unique_ptr<aaltitoad::ntta_t>& ntta, std::map<std::string, argument_t>& cli_arguments);

int main(int argc, char** argv) {
    auto options = get_options();
    auto cli_arguments = get_arguments(options, argc, argv);
    if(cli_arguments["verbosity"])
        spdlog::set_level(static_cast<spdlog::level::level_enum>(SPDLOG_LEVEL_OFF - cli_arguments["verbosity"].as_integer()));
    if(cli_arguments["version"])
        return print_version();
    if(cli_arguments["help"])
        return print_help(argv[0], options);
    if(!is_required_provided(cli_arguments, options))
        return print_required_args();

    spdlog::trace("welcome to {0} v{1}", PROJECT_NAME, PROJECT_VER);
    auto automata = get_ntta(cli_arguments);
    if(cli_arguments["list-instances"]) {
        for(auto& c: automata->components)
            std::cout << c.first << " ";
        std::cout << std::endl;
    } else {
        find_deadlocks(automata, cli_arguments);
    }
    std::cout << "done" << std::endl;
    return 0;
}

auto get_ntta(std::map<std::string, argument_t>& cli_arguments) -> std::unique_ptr<aaltitoad::ntta_t> {
    /// Load plugins
    auto available_plugins = load_plugins(cli_arguments);

    /// Parser related arguments
    auto ignore_list = cli_arguments["ignore"].as_list_or_default({});

    /// Get the parser
    auto selected_parser = cli_arguments["parser"].as_string_or_default("hawk_parser");
    if(!available_plugins.contains(selected_parser) || available_plugins.at(selected_parser).type != plugin_type::parser)
        throw std::logic_error("no such parser available: " + selected_parser);

    /// Parse provided model
    spdlog::trace("parsing with {0} plugin", selected_parser);
    auto parser = std::get<parser_func_t>(available_plugins.at(selected_parser).function);
    ya::timer<unsigned int> t{};
    auto automata = std::unique_ptr<aaltitoad::ntta_t>(parser(cli_arguments["input"].as_list(), ignore_list));
    spdlog::trace("model parsing took {0}ms", t.milliseconds_elapsed());
    return automata;
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

auto get_mentioned_symbols(const expr::syntax_tree_t& expression, const expr::symbol_table_t& symbols) -> expr::symbol_table_t {
    expr::symbol_table_t mentioned{};
    std::visit(ya::overload(
            [&symbols, &mentioned](const expr::identifier_t& r) {
                spdlog::trace("looking for '{0}' in symbols", r.ident);
                mentioned[r.ident] = symbols.at(r.ident);
            },
            [&](const expr::root_t& r) {
                if(!expression.children().empty())
                    mentioned += get_mentioned_symbols(expression.children()[0], symbols);
            },
            [&](const expr::operator_t& r) {
                for(auto& c: expression.children())
                    mentioned += get_mentioned_symbols(c, symbols);
            },
            [](auto&&) {}
    ), static_cast<const expr::underlying_syntax_node_t&>(expression.node));
    return mentioned;
}

void find_deadlocks(const std::unique_ptr<aaltitoad::ntta_t>& ntta, std::map<std::string, argument_t>& cli_arguments) {
    ya::timer<unsigned int> t{};
    aaltitoad::expression_driver c{ntta->symbols, ntta->external_symbols};
    std::vector<expr::syntax_tree_t> extra_conditions{};
    for(auto& condition : cli_arguments["condition"].as_list_or_default({})) {
        auto result = c.parse(condition);
        if(!result.expression)
            spdlog::error("only raw expressions will be used for extra conditions");
        else
            extra_conditions.push_back(result.expression.value());
    }
    if(cli_arguments["condition-file"]) {
        std::ifstream f(cli_arguments["condition-file"].as_string());
        nlohmann::json data = nlohmann::json::parse(f);
        for(auto& condition : data["conditions"]) {
            auto result = c.parse(condition);
            if(!result.expression)
                spdlog::error("only raw expressions will be used for extra conditions");
            else
                extra_conditions.push_back(result.expression.value());
        }
    }
    spdlog::trace("parsing extra {0} conditions took {1}ms", extra_conditions.size(), t.milliseconds_elapsed());

    t.start();
    expr::symbol_table_t known_symbols{};
    for(auto& k : cli_arguments["known"].as_list_or_default({}))
        known_symbols += c.parse(k).get_symbol_table();
    if(cli_arguments["known-file"]) {
        std::ifstream f(cli_arguments["known-file"].as_string());
        nlohmann::json data = nlohmann::json::parse(f);
        for(auto& known : data["known"])
            known_symbols += c.parse(known).get_symbol_table();
    }
    spdlog::trace("parsing {0} known symbols took {1}ms", known_symbols.size(), t.milliseconds_elapsed());

    t.start();
    expr::symbol_table_t unknown_symbols{};
    auto instances = cli_arguments["instance"].as_list_or_default({});
    if(cli_arguments["instance-file"]) {
        std::ifstream f(cli_arguments["instance-file"].as_string());
        nlohmann::json data = nlohmann::json::parse(f);
        for(auto& instance : data["instances"])
            instances.push_back(instance);
    }
    for(auto& instance: instances) {
        spdlog::trace("looking for '{0}' in components", instance);
        for(auto& location: ntta->components.at(instance).graph->nodes)
            for(auto& edge: location.second.outgoing_edges)
                unknown_symbols += get_mentioned_symbols(edge->second.data.guard, ntta->symbols + ntta->external_symbols);
    }
    spdlog::trace("finding {0} mentioned symbols in {1} tta instances took {2}ms", unknown_symbols.size(), instances.size(), t.milliseconds_elapsed());

    // prune known symbols from unknown
    for(auto& known : known_symbols)
        unknown_symbols.erase(known.first);

    aaltitoad::expression_driver d{known_symbols, unknown_symbols};
    for(auto& instance : instances) {
        spdlog::trace("looking for '{0}' in components", instance);
        for(auto& location : ntta->components.at(instance).graph->nodes) {
            t.start();
            if(location.second.outgoing_edges.empty())
                continue;
            auto not_all_expr = expr::syntax_tree_t{expr::symbol_value_t{true}};
            for(auto& edge: location.second.outgoing_edges)
                not_all_expr = expr::syntax_tree_t{expr::operator_t{expr::operator_type_t::_and}}
                        .concat(not_all_expr)
                        .concat(expr::syntax_tree_t{expr::operator_t{expr::operator_type_t::_not}}.concat(edge->second.data.guard));
            for(auto& condition : extra_conditions)
                not_all_expr = expr::syntax_tree_t{expr::operator_t{expr::operator_type_t::_and}}
                        .concat(not_all_expr)
                        .concat(condition);
            try {
                auto result = d.sat_check(not_all_expr);
                if(!result.empty() || result.get_delay_amount().has_value())
                    std::cout << "[possible deadlock in " << instance << "](location:"
                              << location.second.data.identifier << ") in case:\n"
                              << result << "\n";
            } catch (std::domain_error& e) {
                spdlog::trace(std::string{"domain error: "} + e.what());
            }
            spdlog::trace("{0}::{1} took {2}ms", instance, location.second.data.identifier, t.milliseconds_elapsed());
        }
    }
}
