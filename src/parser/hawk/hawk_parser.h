#ifndef AALTITOAD_HAWK_PARSER_H
#define AALTITOAD_HAWK_PARSER_H
#include "plugin_system/plugin_system.h"
#include "ntta/builder/ntta_builder.h"
#include <nlohmann/json.hpp>

namespace aaltitoad::hawk {
    auto should_ignore(const std::filesystem::directory_entry& entry, const std::vector<std::string>& ignore_list) -> bool;
    auto should_ignore(const std::filesystem::directory_entry& entry, const std::string& ignore_regex) -> bool;
    auto load_part(const nlohmann::json& json_file) -> std::string;
    auto load(const std::vector<std::string>& filepaths, const std::vector<std::string> &ignore_list) -> aaltitoad::ntta_t*;
}

#endif //AALTITOAD_HAWK_PARSER_H
