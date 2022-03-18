#ifndef AALTITOAD_H_UPPAAL_PARSER_H
#define AALTITOAD_H_UPPAAL_PARSER_H
#include <aaltitoadpch.h>
#include <nlohmann/json.hpp>
#include "runtime/ntta.h"

struct h_uppaal_parser_t {
    static ntta_t parse_files(const std::string& filepath, const std::vector<std::string>& ignore_list = {});
private:
    static bool is_component(const nlohmann::json& json);
    static component_t parse_component(const nlohmann::json& json);

    static bool is_symbols(const nlohmann::json& json);
    static symbol_table_t parse_symbols(const nlohmann::json& json);

    static symbol_value_t parse_symbol(const nlohmann::json& json);
};

#endif //AALTITOAD_H_UPPAAL_PARSER_H
