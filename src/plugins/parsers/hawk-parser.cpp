#include "hawk-parser.h"

/// Keys to check for in the model file(s)
constexpr const char* initial_location = "initial_location";
constexpr const char* final_location = "final_location";
constexpr const char* locations = "locations";
constexpr const char* declarations = "declarations";
constexpr const char* immediacy = "urgency";
constexpr const char* immediate = "URGENT";
constexpr const char* edges = "edges";
constexpr const char* name = "name";
constexpr const char* source_location = "source_location";
constexpr const char* target_location = "target_location";
constexpr const char* guard = "guard";
constexpr const char* update = "update";
constexpr const char* symbols = "parts";

ntta_t* hawk_parser_t::parse_folders(const std::vector<std::string> &folder_paths, const std::vector<std::string> &ignore_list) {
    symbol_table_t symbol_table{};
    std::unordered_map<std::string, nlohmann::json> templates{};
    for(const auto& filepath : folder_paths) {
        for (const auto &entry: std::filesystem::directory_iterator(filepath)) {
            try {
                if (std::find(ignore_list.begin(), ignore_list.end(), entry.path().c_str()) != ignore_list.end())
                    continue;
                std::ifstream ifs(entry.path());
                auto json = nlohmann::json::parse(ifs);
                if(is_template(json))
                    templates[json[name]] = json;
                if(is_symbols(json))
                    symbol_table += {}; // TODO: Implement parse_symbols(json);
            } catch (std::exception &e) {
                spdlog::error("Unable to parse json file {0}: {1}", entry.path().c_str(), e.what());
                throw e;
            }
        }
    }
    /// ==== Verification of syntax step ====
    /// TODO: Composition should be performed recursively.
    /// TODO: Because of this, we should check for loops in the declarations
    /// TODO: (Do this with a dependency graph, and check for loops in there)
    /// TODO: If a loop is found, we should simply throw a syntax_error.
    // std::unordered_map<std::string, nlohmann::json> composed_components{};
    /// NOTE: Remember to think in terms of _input syntax_.
    /// ==== Composition Step ====
    /// For each template t:
    ///   For each subcomponent s in t:
    ///     if |preset(s)| > 0:
    ///       sequential_compose(s)
    ///     else if |postset(s)| > 0:
    ///       throw syntax_error("Cannot compose {s} sequentially")
    ///     else
    ///       parallel_compose(s)
    component_map_t components{};
    return new ntta_t(state_t{components, symbol_table});
}

bool hawk_parser_t::is_template(const nlohmann::json &json) {
    return json.contains(locations) &&
           json.contains(edges) &&
           json.contains(name) &&
           json.contains(initial_location) &&
           json.contains(final_location);
}

bool hawk_parser_t::is_symbols(const nlohmann::json &json) {
    return json.contains(symbols);
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
