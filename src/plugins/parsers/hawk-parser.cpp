#include <timer>
#include <drivers/interpreter.h>
#include "hawk-parser.h"
#include "extensions/graph_algorithms"
#include "hawk-syntax-layers/hawk-layers.h"

ntta_t* hawk_parser_t::parse_folders(const std::vector<std::string>& folder_paths, const std::vector<std::string>& ignore_list) {
    spdlog::debug("==== HAWK ====");
    ya::timer<unsigned int> t{};
    auto on_apply_call = [](const auto& layer_name){ spdlog::debug("=====<{0}>=====", layer_name); };
    value_layer_stack<template_symbol_collection_t> parser_stack{on_apply_call};
    parser_stack.add_layer<file_parser_layer>(folder_paths, ignore_list);
    parser_stack.add_layer<composition_check_layer>();
    parser_stack.add_layer<parallel_composition_layer>();
    parser_stack.add_layer<sequential_composition_layer>();
    parser_stack.add_layer<parameterization_layer>();
    auto ret = from_syntax(parser_stack.apply());
    spdlog::debug("====/HAWK ==== ({0}ms)", t.milliseconds_elapsed());
    return ret;
}

ntta_t* hawk_parser_t::from_syntax(const template_symbol_collection_t& syntax) {
    spdlog::debug("=====<parse_syntax_as_ntta>=====");
    spdlog::info("Converting syntax into ntta_t");
    expr::symbol_table_t symbol_table{syntax.symbols};
    component_map_t components{};
    for(auto& component : syntax.map) {
        try {
            spdlog::trace("Parsing component {0}", component.first);
            components.insert({component.first, parse_component(component.second)});
            symbol_table += parse_component_declarations(component.second);
        } catch (std::exception& e) {
            spdlog::error("Unable to convert syntax to ntta {0}", e.what());
            throw e;
        }
    }
    return new ntta_t{{components,symbol_table}};
}

component_t hawk_parser_t::parse_component(const nlohmann::json& json) {
    using namespace syntax_constants;
    location_map_t location_list{};
    for(auto& location : json[locations])
        location_list.insert({location["id"], location_t{location[immediacy] == immediate}});
    location_list.insert({json[initial_location]["id"], location_t{}});
    location_list.insert({json[final_location]["id"], location_t{}});

    edge_list_t edge_list{};
    for(auto& edge : json[edges])
        edge_list.emplace_back(edge[source_location], edge[target_location], edge[guard], edge[update]);
    edge_list.emplace_back(json[final_location]["id"], json[initial_location]["id"], "true", "");
    return component_t{std::move(location_list), std::move(edge_list), json[initial_location]["id"]};
}

expr::symbol_table_t hawk_parser_t::parse_component_declarations(const nlohmann::json& json) {
    using namespace syntax_constants;
    if(!json.contains(declarations))
        return {};
    expr::interpreter drv{{}};
    auto res = drv.parse(json[declarations]);
    if(res != 0)
        throw std::logic_error(std::string("Unable to evaluate declaration expression ") + std::string(json[declarations]));
    return drv.result;
}

extern "C" {
    const char *get_plugin_name() {
        return "hawk_parser";
    }

    const char* get_plugin_version() {
        return PLUGIN_VERSION;
    }

    unsigned int get_plugin_type() {
        return static_cast<unsigned int>(plugin_type::parser);
    }

    ntta_t* load(const std::vector<std::string> &folders, const std::vector<std::string> &ignore_list) {
        return hawk_parser_t::parse_folders(folders, ignore_list);
    }
}
