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
#include "expr-wrappers/interpreter.h"
#include "util/warnings.h"
#include <aaltitoadpch.h>
#include <config.h>
#include <lyra/lyra.hpp>
#include <nlohmann/json.hpp>
#include <plugin_system/plugin_system.h>
#include <timer>

auto get_ntta(const std::vector<std::string>& plugin_dirs, const std::vector<std::string>& ignore_list, const std::string& parser, const std::vector<std::string>& input) -> aaltitoad::ntta_t;
void find_deadlocks(const aaltitoad::ntta_t& ntta, const std::vector<std::string>& conditions, const std::optional<std::string>& condition_file, const std::vector<std::string> knowns, const std::optional<std::string>& known_file, const std::vector<std::string>& instance, const std::optional<std::string>& instance_file);

int main(int argc, char** argv) {
    bool show_help = false;
    std::vector<std::string> input = {};
    int verbosity = SPDLOG_LEVEL_INFO;
    bool show_version = false;
    std::vector<std::string> ignore = {};
    std::vector<std::string> plugin_dirs = {};
    bool list_plugins = false;
    std::string parser = "huppaal_parser";
    std::vector<std::string> instances = {};
    std::optional<std::string> instance_file = {};
    bool list_instances = false;
    std::vector<std::string> known_declarations = {};
    std::optional<std::string> known_file = {};
    std::vector<std::string> condition = {};
    std::optional<std::string> condition_file = {};
    auto cli = lyra::cli()
        | lyra::help(show_help).description("Tool to check for non-determinism in a NTTA")
            ("show this message")
        | lyra::opt(input, "DIR")
            ["-f"]["--input"]("add input directory to parse and simulate").required()
        | lyra::opt(verbosity, "0-6")
            ["-v"]["--verbosity"]("set verbosity level, default: " + std::to_string(verbosity))
        | lyra::opt(show_version)
            ["-V"]["--version"]("show version")
        | lyra::opt(ignore, "GLOB")
            ["-i"]["--ignore"]("add a glob file pattern to ignore")
        | lyra::opt(plugin_dirs, "DIR")
            ["-d"]["--plugin-dir"]("use a directory to look for plugins")
        | lyra::opt(list_plugins)
            ["-L"]["--list-plugins"]("list found plugins and exit")
        | lyra::opt(parser, "PARSER")
            ["-p"]["--parser"]("the parser to use")
        | lyra::opt(instances, "INSTANCE")
            ["-n"]["--instance"]("add a TTA instance to check")
        | lyra::opt(instance_file, "FILE")
            ["-N"]["--instance-file"]("set a json file with TTA instances to check")
        | lyra::opt(list_instances)
            ["-L"]["--list-instances"]("list available instances and exit")
        | lyra::opt(known_declarations, "EXPR")
            ["-k"]["--known"]("set known symbols declaration expression")
        | lyra::opt(known_file, "FILE")
            ["-K"]["--known-file"]("set a json file with known symbol declarations")
        | lyra::opt(condition, "EXPR")
            ["-c"]["--condition"]("set the condition to check")
        | lyra::opt(condition_file, "FILE")
            ["-C"]["--condition-file"]("set a json file with conditions to check")
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

    auto automata = get_ntta(plugin_dirs, ignore, parser, input);
    if(list_instances) {
        for(auto& c: automata.components)
            std::cout << c.first << " ";
        std::cout << std::endl;
        return 0;
    }

    spdlog::trace("looking for deadlocks");
    find_deadlocks(automata, condition, condition_file, known_declarations, known_file, instances, instance_file);
    return 0;
}

auto get_ntta(const std::vector<std::string>& plugin_dirs, const std::vector<std::string>& ignore_list, const std::string& parser, const std::vector<std::string>& input) -> aaltitoad::ntta_t {
    /// Load plugins
    auto available_plugins = aaltitoad::plugins::load(plugin_dirs);

    /// Get the parser
    if(!available_plugins.contains(parser) || available_plugins.at(parser).type != plugin_type::parser)
        throw std::logic_error("no such parser available: " + parser);

    /// Parse provided model
    spdlog::trace("parsing with {0} plugin", parser);
    auto p = std::get<parser_ctor_t>(available_plugins.at(parser).function)();
    ya::timer<unsigned int> t{};
    auto parse_result = p->parse_files(input, ignore_list);
    aaltitoad::warnings::print_warnings(parse_result);
    if(!parse_result.has_value())
        throw std::logic_error("compilation failed");
    spdlog::trace("model parsing took {0}ms", t.milliseconds_elapsed());
    return parse_result.value().ntta;
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

void find_deadlocks(const aaltitoad::ntta_t& ntta, const std::vector<std::string>& conditions, const std::optional<std::string>& condition_file, const std::vector<std::string> knowns, const std::optional<std::string>& known_file, const std::vector<std::string>& instance, const std::optional<std::string>& instance_file) {
    ya::timer<unsigned int> t{};
    aaltitoad::expression_driver c{ntta.symbols, ntta.external_symbols};
    std::vector<expr::syntax_tree_t> extra_conditions{};
    for(auto& condition : conditions) {
        auto result = c.parse(condition);
        if(!result.expression)
            spdlog::error("only raw expressions will be used for extra conditions");
        else
            extra_conditions.push_back(result.expression.value());
    }
    if(condition_file.has_value()) {
        std::ifstream f(condition_file.value());
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
    for(auto& k : knowns)
        known_symbols += c.parse(k).get_symbol_table();
    if(known_file.has_value()) {
        std::ifstream f(known_file.value());
        nlohmann::json data = nlohmann::json::parse(f);
        for(auto& known : data["known"])
            known_symbols += c.parse(known).get_symbol_table();
    }
    spdlog::trace("parsing {0} known symbols took {1}ms", known_symbols.size(), t.milliseconds_elapsed());

    t.start();
    expr::symbol_table_t unknown_symbols{};
    auto instances = instance;
    if(instance_file.has_value()) {
        std::ifstream f(instance_file.value());
        nlohmann::json data = nlohmann::json::parse(f);
        for(auto& instance : data["instances"])
            instances.push_back(instance);
    }
    for(auto& instance: instances) {
        spdlog::trace("looking for '{0}' in components", instance);
        for(auto& location: ntta.components.at(instance).graph->nodes)
            for(auto& edge: location.second.outgoing_edges)
                unknown_symbols += get_mentioned_symbols(edge->second.data.guard, ntta.symbols + ntta.external_symbols);
    }
    spdlog::trace("finding {0} mentioned symbols in {1} tta instances took {2}ms", unknown_symbols.size(), instances.size(), t.milliseconds_elapsed());

    // prune known symbols from unknown
    for(auto& known : known_symbols)
        unknown_symbols.erase(known.first);

    aaltitoad::expression_driver d{known_symbols, unknown_symbols};
    for(auto& instance : instances) {
        spdlog::trace("looking for '{0}' in components", instance);
        for(auto& location : ntta.components.at(instance).graph->nodes) {
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
