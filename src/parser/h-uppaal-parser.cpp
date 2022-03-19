#include "h-uppaal-parser.h"
#include "parser/driver.h"
#include <nlohmann/json.hpp>

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

ntta_t h_uppaal_parser_t::parse_folder(const std::string& filepath, const std::vector<std::string>& ignore_list) {
    symbol_table_t symbol_table{};
    component_map_t components{};
    for (const auto & entry : std::filesystem::directory_iterator(filepath)) {
        try {
            if(std::find(ignore_list.begin(), ignore_list.end(), entry.path().c_str()) != ignore_list.end())
                continue;

            std::ifstream ifs(entry.path());
            auto jf = nlohmann::json::parse(ifs);

            if(is_component(jf)) {
                spdlog::debug("Parsing component {0}", entry.path().c_str());
                components.insert({jf[name], parse_component(jf)});
                symbol_table += parse_component_declarations(jf);
            } else if(is_symbols(jf)) {
                spdlog::debug("Parsing symbols {0}", entry.path().c_str());
                symbol_table += parse_symbols(jf);
            }
        } catch(std::exception& e) {
            spdlog::error("Unable to parse json file {0}: {1}", entry.path().c_str(), e.what());
            throw e;
        }
    }

    return ntta_t(state_t{components, symbol_table});
}

bool h_uppaal_parser_t::is_component(const nlohmann::json& json) {
    return json.contains(locations) &&
           json.contains(edges) &&
           json.contains(name) &&
           json.contains(initial_location) &&
           json.contains(final_location);
}

bool h_uppaal_parser_t::is_symbols(const nlohmann::json& json) {
    return json.contains(symbols);
}

component_t h_uppaal_parser_t::parse_component(const nlohmann::json& json) {
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

symbol_table_t h_uppaal_parser_t::parse_component_declarations(const nlohmann::json& json) {
    if(!json.contains(declarations))
        return {};
    driver drv{{}};
    auto res = drv.parse(json[declarations]);
    if (res)
        throw std::logic_error(&"Unable to evaluate declaration expression "[json[declarations]]);
    return drv.result;
}

symbol_table_t h_uppaal_parser_t::parse_symbols(const nlohmann::json& json) {
    symbol_table_t symbol_table{};
    for(auto& symbol : json[symbols])
        symbol_table[symbol["PartName"]] = parse_symbol((*symbol["GenericType"].begin())["Value"]);
    return symbol_table;
}

symbol_value_t h_uppaal_parser_t::parse_symbol(const nlohmann::json& json) {
    if(json.is_number_float())
        return (float)json;
    if(json.is_number())
        return (int)json;
    if(json.is_boolean())
        return (bool)json;
    if(json.is_string())
        return (std::string)json;
    throw std::logic_error((std::stringstream{} << "Not a symbol literal: " << json).str());
}
