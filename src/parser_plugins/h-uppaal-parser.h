#ifndef AALTITOAD_H_UPPAAL_PARSER_H
#define AALTITOAD_H_UPPAAL_PARSER_H
#include <aaltitoadpch.h>
#include <nlohmann/json.hpp>
#include "runtime/ntta.h"

struct h_uppaal_parser_t {
    static ntta_t parse_folder(const std::vector<std::string>& folder_paths, const std::vector<std::string>& ignore_list = {});
private:
    static bool is_component(const nlohmann::json& json);
    static component_t parse_component(const nlohmann::json& component);
    static symbol_table_t parse_component_declarations(const nlohmann::json& component);

    static bool is_symbols(const nlohmann::json& json);
    static symbol_table_t parse_symbols(const nlohmann::json& symbols);
    static symbol_value_t parse_symbol(const nlohmann::json& symbol);
};

extern "C" const char* get_plugin_name();
extern "C" ntta_t h_uppaal_parser_load(const std::vector<std::string>& folders, const std::vector<std::string>& ignore_list);

#endif //AALTITOAD_H_UPPAAL_PARSER_H
