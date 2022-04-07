#ifndef AALTITOAD_PLUGIN_SYSTEM_H
#define AALTITOAD_PLUGIN_SYSTEM_H
#include <dlfcn.h>
#include <runtime/tocker.h>
#include <runtime/ntta.h>

enum class plugin_type : unsigned int {
    tocker = 0,
    parser
};
using get_plugin_name_t = const char*(*)();
using get_plugin_type_t = unsigned int(*)();
using tocker_ctor_t = tocker_t*(*)(const std::string&, const ntta_t&);
using parser_func_t = ntta_t*(*)(const std::vector<std::string>&, const std::vector<std::string>&);
using plugin_function_t = std::variant<tocker_ctor_t, parser_func_t>;
using plugin_t = std::pair<plugin_type, plugin_function_t>;

using plugin_map_t = std::map<std::string, plugin_t>;

namespace plugins {
    plugin_map_t load(const std::vector<std::string> &search_directories);
}

#endif //AALTITOAD_PLUGIN_SYSTEM_H
