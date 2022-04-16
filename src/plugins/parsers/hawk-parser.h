#ifndef AALTITOAD_HAWK_PARSER_H
#define AALTITOAD_HAWK_PARSER_H
#include "plugin_system/plugin_system.h"
#include <nlohmann/json.hpp>

using template_map = std::unordered_map<std::string, nlohmann::json>;
struct template_symbol_collection_t {
    symbol_table_t symbols;
    template_map map;
};

class hawk_parser_t {
public:
    static ntta_t* parse_folders(const std::vector<std::string>& folder_paths, const std::vector<std::string>& ignore_list);

private:
    static ntta_t* from_syntax(const template_symbol_collection_t& syntax);
    static component_t parse_component(const nlohmann::json& component);
    static symbol_table_t parse_component_declarations(const nlohmann::json& component);
};

extern "C" {
    const char* get_plugin_name();
    unsigned int get_plugin_type();
    ntta_t* load(const std::vector<std::string>& folders, const std::vector<std::string>& ignore_list);
}

#endif
