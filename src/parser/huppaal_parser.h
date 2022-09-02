#ifndef AALTITOAD_HUPPAAL_PARSER_H
#define AALTITOAD_HUPPAAL_PARSER_H
#include "plugin_system/plugin_system.h"
#include "ntta/ntta_builder.h"
#include <nlohmann/json.hpp>

namespace aaltitoad::huppaal {
    auto load_declarations(const nlohmann::json& json_file, const expr::symbol_table_t& symbols) -> expr::symbol_table_t;
    auto load_part(const nlohmann::json& json_file) -> expr::symbol_table_t;
    auto load(const std::vector<std::string>& filepaths, const std::vector<std::string> &ignore_list) -> aaltitoad::ntta_t*;
    auto load_tta(const nlohmann::json& json_file, expr::symbol_table_t& symbols) -> aaltitoad::tta_builder;
}

#endif //AALTITOAD_HUPPAAL_PARSER_H
