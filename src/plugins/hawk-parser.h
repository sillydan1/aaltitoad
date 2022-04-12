#ifndef AALTITOAD_HAWK_PARSER_H
#define AALTITOAD_HAWK_PARSER_H
#include "plugin_system/plugin_system.h"
#include <nlohmann/json.hpp>

class hawk_parser_t {
    static ntta_t* parse_folder(const std::vector<std::string>& folder_paths, const std::vector<std::string>& ignore_list = {});
private:
    static bool is_component(const nlohmann::json& json);
    static component_t parse_component(const nlohmann::json& component);
    static symbol_table_t parse_component_declarations(const nlohmann::json& component);

    static bool is_symbols(const nlohmann::json& json);
    static symbol_table_t parse_symbols(const nlohmann::json& symbols);
    static symbol_value_t parse_symbol(const nlohmann::json& symbol);
};

extern "C" {
    const char* get_plugin_name();
    unsigned int get_plugin_type();
    ntta_t* load(const std::vector<std::string>& folders, const std::vector<std::string>& ignore_list);
}

#endif //AALTITOAD_HAWK_PARSER_H
