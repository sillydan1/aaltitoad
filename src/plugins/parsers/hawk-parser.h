#ifndef AALTITOAD_HAWK_PARSER_H
#define AALTITOAD_HAWK_PARSER_H
#include "plugin_system/plugin_system.h"
#include <nlohmann/json.hpp>

class hawk_parser_t {
public:
    static ntta_t* parse_folders(const std::vector<std::string>& folder_paths, const std::vector<std::string>& ignore_list = {});

private:
    static bool is_component(const nlohmann::json&);
};

extern "C" {
    const char* get_plugin_name();
    unsigned int get_plugin_type();
    ntta_t* load(const std::vector<std::string>& folders, const std::vector<std::string>& ignore_list);
}

#endif //AALTITOAD_HAWK_PARSER_H
