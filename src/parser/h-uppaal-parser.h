#ifndef AALTITOAD_H_UPPAAL_PARSER_H
#define AALTITOAD_H_UPPAAL_PARSER_H
#include <aaltitoadpch.h>
#include <nlohmann/json.hpp>
#include "runtime/ntta.h"

struct h_uppaal_parser_t {
    /// Attempts to parse all files in provided folder path as an ntta_t
    /// \param folder_path Path to the folder containing ntta json files
    /// \param ignore_list A list of files to ignore
    /// \return A fully parsed ntta_t
    /// \throws std::exception If a file is not a valid ntta_t json file
    static ntta_t parse_folder(const std::string& folder_path, const std::vector<std::string>& ignore_list = {});
private:
    static bool is_component(const nlohmann::json& json);
    static component_t parse_component(const nlohmann::json& component);
    static symbol_table_t parse_component_declarations(const nlohmann::json& component);

    static bool is_symbols(const nlohmann::json& json);
    static symbol_table_t parse_symbols(const nlohmann::json& symbols);
    static symbol_value_t parse_symbol(const nlohmann::json& symbol);
};

#endif //AALTITOAD_H_UPPAAL_PARSER_H
