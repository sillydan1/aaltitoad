#ifndef AALTITOAD_HAWK_PARSER_H
#define AALTITOAD_HAWK_PARSER_H
#include "plugin_system/plugin_system.h"
#include <nlohmann/json.hpp>

class hawk_parser_t {
public:
    static ntta_t* parse_folders(const std::vector<std::string>& folder_paths, const std::vector<std::string>& ignore_list);
private:
    using template_map = std::unordered_map<std::string, nlohmann::json>;
    static bool is_template(const nlohmann::json& json);
    static bool is_symbols(const nlohmann::json& json);
    static symbol_table_t parse_symbols(const nlohmann::json& symbols);
    static symbol_value_t parse_symbol(const nlohmann::json& symbol);

    static void check_for_invalid_subcomponent_composition(const template_map& templates);
};

extern "C" {
    const char* get_plugin_name();
    unsigned int get_plugin_type();
    ntta_t* load(const std::vector<std::string>& folders, const std::vector<std::string>& ignore_list);
}

#endif
