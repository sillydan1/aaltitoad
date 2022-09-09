#ifndef AALTITOAD_HUPPAAL_PARSER_H
#define AALTITOAD_HUPPAAL_PARSER_H
#include "plugin_system/plugin_system.h"
#include "ntta/builder/ntta_builder.h"
#include "ntta/builder/ntta_builder_2.h"
#include <nlohmann/json.hpp>

namespace aaltitoad::huppaal {
    void load_declarations(ntta_builder2& b, const nlohmann::json& json_file);
    auto should_ignore(const std::filesystem::directory_entry& entry, const std::vector<std::string>& ignore_list) -> bool;
    auto should_ignore(const std::filesystem::directory_entry& entry, const std::string& ignore_regex) -> bool;
    auto load_part(const nlohmann::json& json_file) -> std::string;
    auto load(const std::vector<std::string>& filepaths, const std::vector<std::string> &ignore_list) -> aaltitoad::ntta_t*;
    auto load_tta(const nlohmann::json& json_file) -> aaltitoad::tta_builder2;
}

#endif //AALTITOAD_HUPPAAL_PARSER_H
