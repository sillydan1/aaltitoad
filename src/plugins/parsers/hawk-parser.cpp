#include "hawk-parser.h"
#include "extensions/graph_algorithms"
#include "hawk-syntax-layers/hawk-layers.h"

ntta_t* hawk_parser_t::parse_folders(const std::vector<std::string>& folder_paths, const std::vector<std::string>& ignore_list) {
    spdlog::debug("==== HAWK ====");
    auto on_apply_call = [](const auto& layer_name){ spdlog::trace("Engaging layer '{0}'", layer_name); };
    value_layer_stack<template_symbol_collection_t> parser_stack{on_apply_call};
    parser_stack.add_layer<file_parser_layer>(folder_paths, ignore_list);
    parser_stack.add_layer<composition_check_layer>();
    auto collection = parser_stack.apply();

    spdlog::debug("====/HAWK ====");
    return new ntta_t{{{},{}}}; // TODO: from template_symbol_collection_t to ntta*
}
/*
auto hawk_parser_t::compose_parallel(const nlohmann::json &template_json,
                                     const std::string &component_name,
                                     const template_map &templates) -> template_map {
    /// ==== Composition Step ====
    /// For each template t:
    for(auto& t : templates) {
        ///   For each subcomponent s in t:
        for(auto& s : t.second[sub_components]) {
            value_layer_stack<template_map> syntax_steps{};

            // auto pre  = preset(template_json, s[name]);
            // auto post = postset(template_json, s[name]);
            ///     if |preset(s)| > 0:
            ///       sequential_compose(s)
            ///     else if |postset(s)| > 0:
            ///       throw syntax_error("Cannot compose {s} sequentially")
            ///     else
            ///       parallel_compose(s)
        }
    }
    return {};
}
*/
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
