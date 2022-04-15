#include "hawk-parser.h"
#include "extensions/graph_algorithms"
#include <ranges>
#include <regex>
#include <extensions/map_extensions.h>
#include <Timer.hpp>

/// Keys to check for in the model file(s)
constexpr const char* initial_location = "initial_location";
constexpr const char* final_location = "final_location";
constexpr const char* locations = "locations";
constexpr const char* declarations = "declarations";
constexpr const char* sub_components = "sub_components";
constexpr const char* immediacy = "urgency";
constexpr const char* immediate = "URGENT";
constexpr const char* edges = "edges";
constexpr const char* name = "name";
constexpr const char* source_location = "source_location";
constexpr const char* target_location = "target_location";
constexpr const char* guard = "guard";
constexpr const char* update = "update";
constexpr const char* symbols = "parts";

ntta_t* hawk_parser_t::parse_folders(const std::vector<std::string>& folder_paths, const std::vector<std::string>& ignore_list) {
    symbol_table_t symbol_table{};
    template_map templates{};
    for(const auto& filepath : folder_paths) {
        for (const auto &entry: std::filesystem::directory_iterator(filepath)) {
            try {
                if(std::find_if(ignore_list.begin(), ignore_list.end(),
                    [&entry](const auto& i) {
                        return std::regex_match(entry.path().c_str(), std::regex{i, std::regex::extended});
                    }) != ignore_list.end())
                    continue;
                std::ifstream ifs(entry.path());
                auto json = nlohmann::json::parse(ifs);
                if(is_template(json)) {
                    if(templates.contains(json[name]))
                        throw std::logic_error("Multiple definitions of component template");
                    templates[json[name]] = json;
                }
                if(is_symbols(json))
                    symbol_table += {}; // TODO: Implement parse_symbols(json);
            } catch (std::exception &e) {
                spdlog::error("Unable to parse json file {0}: {1}", entry.path().c_str(), e.what());
                throw e;
            }
        }
    }
    /// ==== Verification of syntax step ====
    check_for_invalid_subcomponent_composition(templates); // throws if invalid component is found
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

void hawk_parser_t::check_for_invalid_subcomponent_composition(const template_map& templates) {
    spdlog::info("Analyzing component template dependencies");
    spdlog::trace("Generating dependency graph");
    Timer<int> t{};
    t.start();
    auto template_names = get_key_set(templates);
    association_graph<std::string> subcomponent_dependency_graph{template_names};
    for(int i = 0; i < template_names.size(); i++) {
        for(auto& j : templates.at(template_names[i])[sub_components]) {
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
