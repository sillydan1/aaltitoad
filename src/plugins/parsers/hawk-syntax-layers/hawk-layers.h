#ifndef AALTITOAD_HAWK_LAYERS_H
#define AALTITOAD_HAWK_LAYERS_H
#include <extensions/function_layer.h>
#include <regex>
#include <extensions/map_extensions.h>
#include <Timer.hpp>
#include <threadpool>
#include "../hawk-parser.h"
#include "extensions/exceptions/ntta_error.h"
#include <parser/interpreter.h>

/// Keys to check for in the model file(s)
namespace syntax_constants {
    constexpr const char *initial_location = "initial_location";
    constexpr const char *final_location = "final_location";
    constexpr const char *locations = "locations";
    constexpr const char *declarations = "declarations";
    constexpr const char *sub_components = "sub_components";
    constexpr const char *immediacy = "urgency";
    constexpr const char *immediate = "URGENT";
    constexpr const char *edges = "edges";
    constexpr const char *name = "name";
    constexpr const char *is_main = "main";
    constexpr const char *source_location = "source_location";
    constexpr const char *target_location = "target_location";
    constexpr const char *guard = "guard";
    constexpr const char *update = "update";
    constexpr const char *symbols = "parts";
}
using syntax_layer = value_layer<template_symbol_collection_t>;
void operator+=(template_map& a, const template_map& b) {
    a.insert(b.begin(), b.end());
}
void operator+=(template_symbol_collection_t& a, const template_map& b) {
    a.map += b;
}
void operator+=(template_symbol_collection_t& a, const template_symbol_collection_t& b) {
    a.map += b.map;
    a.symbols += b.symbols;
}

class file_parser_layer : public syntax_layer {
public:
    file_parser_layer(const std::vector<std::string>& folder_paths, const std::vector<std::string>& ignore_list)
     : syntax_layer{"file_parser_layer"}, folder_paths{folder_paths}, ignore_list{ignore_list} {}
    auto on_call(const template_symbol_collection_t& c) -> template_symbol_collection_t override {
        spdlog::info("Loading input files");
        symbol_table_t symbol_table{};
        template_map templates{};
        Timer<unsigned int> t{}; t.start();
        for(const auto& filepath : folder_paths) {
            for (const auto &entry: std::filesystem::directory_iterator(filepath)) {
                try {
                    if(std::find_if(ignore_list.begin(), ignore_list.end(),
                                    [&entry](const auto& i) {
                                        return std::regex_match(entry.path().c_str(), std::regex{i, std::regex::extended});
                                    }) != ignore_list.end())
                        continue;
                    std::ifstream ifs(entry.path());
                    auto file_contents = load_ignore_comments(ifs);
                    auto json = nlohmann::json::parse(file_contents);
                    if(is_template(json)) {
                        if(templates.contains(json[syntax_constants::name]))
                            throw std::logic_error("Multiple definitions of component template");
                        templates[json[syntax_constants::name]] = json;
                    }
                    if(is_symbols(json)) {
                        spdlog::trace("loading {0} as symbols", entry.path().c_str());
                        symbol_table += parse_symbols(json);
                    }
                } catch (std::exception &e) {
                    spdlog::error("Unable to parse json file {0}: {1}", entry.path().c_str(), e.what());
                    throw e;
                }
            }
        }
        spdlog::trace("Loading files took {0}ms", t.milliseconds_elapsed());
        return template_symbol_collection_t{.symbols=symbol_table, .map=templates};
    }

private:
    const std::vector<std::string>& folder_paths;
    const std::vector<std::string>& ignore_list;

    static bool is_template(const nlohmann::json &json) {
        return json.contains(syntax_constants::locations) &&
               json.contains(syntax_constants::edges) &&
               json.contains(syntax_constants::name) &&
               json.contains(syntax_constants::initial_location) &&
               json.contains(syntax_constants::final_location);
    }
    static bool is_symbols(const nlohmann::json &json) {
        return json.contains(syntax_constants::symbols);
    }

    static auto parse_symbols(const nlohmann::json& json) -> symbol_table_t {
        symbol_table_t symbol_table{};
        for(auto& symbol : json[syntax_constants::symbols])
            symbol_table[symbol["ID"]] = parse_symbol(symbol);
        return symbol_table;
    }

    static auto parse_symbol(const nlohmann::json& json) -> symbol_value_t {
        if(json.contains("Value"))
            return parse_literal(json["Value"]);
        // Custom types:
        auto type = json["Type"];
        if(type == "EMR")
            return 0;
        if(type == "DigitalOutput")
            return false;
        if(type == "DigitalInput")
            return false;
        if(type == "AnalogOutput")
            return 0;
        if(type == "AnalogInput")
            return 0;
        if(type == "HighSpeedCounter")
            return 0;
        if(type == "DigitalToggleSwitch")
            return false;
        if(type == "Timer")
            return 0; // TODO: Add Clock support to ntta_t
        throw std::logic_error((string_builder{} << "Unsupported part type: " << type << " :\n" << std::setw(2) << json));
    }

    static auto parse_literal(const nlohmann::json& json) -> symbol_value_t {
        if(json.is_number_float())
            return (float)json;
        if(json.is_number())
            return (int)json;
        if(json.is_boolean())
            return (bool)json;
        if(json.is_string())
            return (std::string)json;
        throw std::logic_error((string_builder{} << "Not a symbol literal: " << json));
    }

    static auto load_ignore_comments(std::ifstream& ifs) -> std::string {
        bool skip = false;
        std::string str;
        std::string file_contents;
        while (std::getline(ifs, str)) {
            if(contains(str,"/*"))
                skip = true;
            if(skip && contains(str, "*/")) {
                auto x = split(str,"*/");
                str = x[1];
                skip = false;
            }
            if(skip)
                continue;
            file_contents += str;
            file_contents.push_back('\n');
        }
        return file_contents;
    }
};

// TODO: Recursion should be supported (but do it cleverly.)
class composition_check_layer : public syntax_layer {
public:
    composition_check_layer() : syntax_layer{"composition_check_layer"} {}
    auto on_call(const template_symbol_collection_t &t) -> template_symbol_collection_t override {
        check_for_invalid_subcomponent_composition(t.map); // throws if invalid composition is found
        return t;
    }
private:
    static void check_for_invalid_subcomponent_composition(const template_map& templates) {
        spdlog::info("Analyzing component template dependencies");
        spdlog::trace("Generating dependency graph");
        Timer<int> t{};
        t.start();
        auto template_names = get_key_set(templates);
        graph<std::string> subcomponent_dependency_graph{template_names};
        for(int i = 0; i < template_names.size(); i++) {
            for(auto& j : templates.at(template_names[i])[syntax_constants::sub_components]) {
                auto& component_name = j["component"];
                auto tmp = std::find(template_names.begin(), template_names.end(), component_name);
                if(tmp == template_names.end())
                    throw std::logic_error(to_string(component_name) + ": No such component template");
                subcomponent_dependency_graph.insert_edge(i, tmp-template_names.begin());
            }
        }
        spdlog::trace("Dependency graph generation took {0}ms", t.milliseconds_elapsed());
        spdlog::trace("Searching for strongly connected components");
        t.start();
        auto sccs = tarjan(subcomponent_dependency_graph);
        spdlog::trace("SCC generation took {0}ms", t.milliseconds_elapsed());
        spdlog::trace("Looking for cycles in {0} strongly connected components", sccs.size());
        t.start();
        std::vector<std::string> cycles{};
        for(auto& scc : sccs) {
            if(has_cycle_dfs<std::string>(scc)) {
                std::stringstream ss{"Cyclic dependency detected in: [ "};
                for(auto& e : scc)
                    ss << e << " ";
                ss << "]";
                cycles.push_back(ss.str());
            }
        }
        if(!cycles.empty()) {
            for(auto& c : cycles)
                spdlog::error(c);
            throw std::logic_error("Found cyclic dependencies");
        }
        spdlog::trace("Strongly connected component cycle check took {0}ms", t.milliseconds_elapsed());
    }
};

class parallel_composition_layer : public syntax_layer {
public:
    parallel_composition_layer() : syntax_layer("parallel_composition_layer") {}
    auto on_call(const template_symbol_collection_t& templates) -> template_symbol_collection_t override {
        spdlog::info("Composing parallel templates");
        auto main_component_template_it = std::find_if(templates.map.begin(), templates.map.end(),
                                                    [](const auto& e){ return e.second[syntax_constants::is_main]; });
        template_symbol_collection_t return_value{.symbols=templates.symbols};
        nlohmann::json main_component_sub_component = {
                {"component", main_component_template_it->first},
                {"identifier", main_component_template_it->first}
        };
        return_value += parallel_compose(main_component_sub_component, "", templates);
        spdlog::trace("Composed {0} components parallel", return_value.map.size());
        return return_value;
    }

private:
    static auto has_ingoing_edge(const nlohmann::json& parent_edges, const std::string& identifier) {
        return std::any_of(parent_edges.begin(), parent_edges.end(),
                           [&identifier](const auto& j){ return j.at("target_sub_component") == identifier; });
    }
    static auto has_outgoing_edge(const nlohmann::json& parent_edges, const std::string& identifier) {
        return std::any_of(parent_edges.begin(), parent_edges.end(),
                           [&identifier](const auto& j){ return j.at("source_sub_component") == identifier; });
    }
    // We have guaranteed no infinite recursion in the composition_check_layer, so no need to worry
    static auto parallel_compose(const nlohmann::json& sub_component_object, // NOLINT(misc-no-recursion)
                                 const std::string& parent_component,
                                 const template_symbol_collection_t& templates) -> template_symbol_collection_t {
        template_symbol_collection_t return_value{};
        auto this_template_name = sub_component_object["component"];
        auto this_template_identifier = sub_component_object["identifier"];
        return_value.map[this_template_identifier] = templates.map.at(this_template_name);
        return_value.map[this_template_identifier]["template_name"] = this_template_name;
        return_value.map[this_template_identifier]["component_identifier"] = this_template_identifier;
        return_value.map[this_template_identifier]["parent_component"] = parent_component;

        auto& parent_edges = templates.map.at(this_template_name)[syntax_constants::edges];
        for(auto& c : templates.map.at(this_template_name)[syntax_constants::sub_components]) {
            auto sub_component_identifier = c["identifier"];
            auto ident = std::string(c["identifier"]);
            if(!has_ingoing_edge(parent_edges, sub_component_identifier)) {
                if(has_outgoing_edge(parent_edges, sub_component_identifier)) {
                    spdlog::error("Only outgoing edges for subcomponent {0} - not allowed", sub_component_identifier);
                    throw std::logic_error("Invalid parallel composition syntax");
                }
                return_value += parallel_compose(c, this_template_name, templates);
            }
        }
        return return_value;
    }
};

class sequential_composition_layer : public syntax_layer {
public:
    sequential_composition_layer() : syntax_layer("sequential_composition_layer") {}
    auto on_call(const template_symbol_collection_t& templates) -> template_symbol_collection_t override {
        spdlog::info("Composing sequential templates");
        template_symbol_collection_t return_value{.symbols=templates.symbols};
        for(auto& t : templates.map)
            return_value.map.insert(std::make_pair(t.first, sequential_compose(t.second, templates.map)));
        spdlog::trace("Composed {0} components sequentially", return_value.map.size());
        return return_value;
    }

private:
    static auto has_ingoing_edge(const nlohmann::json& parent_edges, const std::string& identifier) {
        return std::any_of(parent_edges.begin(), parent_edges.end(),
                           [&identifier](const auto& j){ return j.at("target_sub_component") == identifier; });
    }
    static void annotate(nlohmann::json& annotation_target, const std::string& annotation_string) {
        annotation_target = annotation_string + "." + std::string(annotation_target);
    }
    static auto sequential_compose(const nlohmann::json& component, const template_map& templates) -> nlohmann::json { // NOLINT(misc-no-recursion)
        auto composed_cpy = component;
        for(auto& s : component["sub_components"]) {
            if(!has_ingoing_edge(component["edges"], s["identifier"]))
                continue;
            auto sub_component = sequential_compose(templates.at(s["component"]), templates);
            // TODO: Also annotate declarations and expressions
            // annotations
            auto sub_component_annotation_string = std::string(component["name"]) + "." + std::string(s["identifier"]);
            annotate(sub_component["initial_location"]["id"], sub_component_annotation_string);
            annotate(sub_component["final_location"]["id"], sub_component_annotation_string);
            for(auto& l : sub_component["locations"]) {
                annotate(l["id"], sub_component_annotation_string);
                composed_cpy["locations"].push_back(l);
            }
            for(auto& e : sub_component["edges"]) {
                annotate(e["source_location"], sub_component_annotation_string);
                annotate(e["target_location"], sub_component_annotation_string);
                composed_cpy["edges"].push_back(e);
            }
        }
        return composed_cpy;
    }
};

class expression_parameterizer : public expr::interpreter {
public:
    std::string expression;
    using parameter_map_t = std::unordered_map<std::string, std::pair<std::string, symbol_value_t>>;
    const parameter_map_t& parameter_mapping{};
    explicit expression_parameterizer(const symbol_table_t& env, const parameter_map_t& mapping)
     : expr::interpreter{env}, parameter_mapping{mapping} {}
    auto parse(const std::string& expr) -> int override {
        expression = expr;
        return expr::interpreter::parse(expr);
    }
    auto get_symbol(const std::string& identifier) -> syntax_tree_t override {
        if(environment.contains(identifier))
            return expr::interpreter::get_symbol(identifier);
        for(auto& key : get_key_set(parameter_mapping)) {
            if(identifier == key) {
                expression = regex_replace_all(expression, std::regex("[^a-zA-Z.]"+identifier), parameter_mapping.at(key).first);
                return syntax_tree_t{parameter_mapping.at(key).second};
            }
            if(contains(identifier, "."+key)) {
                auto parameterized_ident = std::regex_replace(identifier, std::regex("\\."+key), "."+parameter_mapping.at(key).first);
                if(environment.contains(parameterized_ident)) {
                    expression = regex_replace_all(expression, std::regex(identifier), parameterized_ident);
                    return expr::interpreter::get_symbol(parameterized_ident);
                }
            }
        }
        throw std::out_of_range("No parameterization available for identifier: "+identifier);
    }
    void set_symbol(const std::string& identifier, const symbol_value_t& value) override {
        for(auto& key : get_key_set(parameter_mapping)) {
            if(contains(identifier, "."+key)) { // The '.' is very important.
                auto parameterized_ident = std::regex_replace(identifier, std::regex(key), parameter_mapping.at(key).first);
                expr::interpreter::set_symbol(parameterized_ident, value);
                return;
            }
        }
        expr::interpreter::set_symbol(identifier, value);
    }
    void add_tree(const std::string& identifier, const syntax_tree_t& tree) override {
        for(auto& key : get_key_set(parameter_mapping)) {
            if(contains(identifier, "."+key)) { // The '.' is very important.
                auto parameterized_ident = std::regex_replace(identifier, std::regex(key), parameter_mapping.at(key).first);
                expr::interpreter::add_tree(parameterized_ident, tree);
                return;
            }
        }
        expr::interpreter::add_tree(identifier, tree);
    }
};

class parameterization_layer : public syntax_layer {
    static auto get_parameter_map(const nlohmann::json& component, const std::string& component_name) {
        expression_parameterizer::parameter_map_t parameter_argument_mapping{};
        auto get_positional_arguments = [](const std::string& inputstr){
            // Remove "xxx(" and ")"'s
            if(!contains(inputstr,"("))
                return std::vector<std::string>{};
            auto r = std::regex_replace(std::regex_replace(inputstr, std::regex("^.+\\("), ""), std::regex("\\)"), "");
            return split(r, ',');
        };
        auto params = get_positional_arguments(component_name);
        auto args = get_positional_arguments(component["component_identifier"]);
        if(params.size() != args.size())
            throw std::logic_error("Unable to parameterize component. Provided arguments does not match parameters");
        for(int i = 0; i < params.size(); i++)
            parameter_argument_mapping.insert_or_assign(params[i], std::make_pair(args[i], symbol_value_t{} <<= args[i]));
        return parameter_argument_mapping;
    }
    static symbol_table_t get_parameterized_declarations(const nlohmann::json& component, const expression_parameterizer::parameter_map_t& mapping) {
        expression_parameterizer parameterizer{{}, mapping};
        auto decl_res = parameterizer.parse(component[syntax_constants::declarations]);
        if(decl_res != 0)
            throw std::logic_error(std::get<std::string>(parameterizer.error));
        return parameterizer.result;
    }
public:
    parameterization_layer() : syntax_layer("parameterization_layer") {}
    auto on_call(const template_symbol_collection_t& components) -> template_symbol_collection_t override {
        spdlog::info("Parameterizing components");
        template_symbol_collection_t return_value{.symbols=components.symbols,.map=components.map};
        spdlog::trace("parameterize and load declarations");
        for(auto& component : return_value.map) {
            auto parameter_argument_mapping = get_parameter_map(component.second, component.second.at("template_name"));
            return_value.symbols += get_parameterized_declarations(component.second, parameter_argument_mapping);
        }
        spdlog::trace("parameterize and load expressions in edges");
        int expression_res = 0;
        for(auto& component : return_value.map) {
            auto parameter_argument_mapping = get_parameter_map(component.second,
                                                                component.second.at("template_name"));
            expression_parameterizer parameterizer{return_value.symbols, parameter_argument_mapping};
            for (auto &edge: component.second["edges"]) {
                auto update = std::string(edge[syntax_constants::update]);
                auto guard = std::string(edge[syntax_constants::guard]);
                auto update_res = parameterizer.parse(update);
                edge[syntax_constants::update] = parameterizer.expression;
                if (update_res != 0) {
                    auto err = std::get<std::string>(parameterizer.error);
                    spdlog::error(ntta_error::update_err_format_spdlog(),
                                  component.first,
                                  edge[syntax_constants::source_location],
                                  edge[syntax_constants::target_location],
                                  update, err);
                }

                auto guard_res = parameterizer.parse(guard);
                edge[syntax_constants::guard] = parameterizer.expression;
                if (guard_res != 0) {
                    auto err = std::get<std::string>(parameterizer.error);
                    spdlog::error(ntta_error::guard_err_format_spdlog(),
                                  component.first,
                                  edge[syntax_constants::source_location],
                                  edge[syntax_constants::target_location],
                                  guard, err);
                }

                expression_res += update_res;
                expression_res += guard_res;
            }
        }
        if(expression_res != 0)
            throw std::logic_error("Errors in expressions");
        spdlog::info("Finished");
        return return_value;
    }
};

#endif //AALTITOAD_HAWK_LAYERS_H
