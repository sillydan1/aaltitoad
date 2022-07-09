#ifndef AALTITOAD_PLUGIN_SYSTEM_H
#define AALTITOAD_PLUGIN_SYSTEM_H
#include <dlfcn.h>
#include <runtime/tocker.h>
#include <runtime/ntta.h>

//// ===== aaltitoad plugin system =====
//// must implement the following extern
//// C function symbols:
////   - const char* get_plugin_name()
////   - const char* get_plugin_version()
////   - plugin_type get_plugin_type()
//// Depending on the type, the plugin
//// should also implement:
////   - tockers:
////     - tocker_t* create_tocker(const std::string&, const ntta_t&)
////   - parsers:
////     - ntta_t* load(const std::vector<std::string>&, const std::vector<std::string>&)
////
enum class plugin_type : unsigned int {
    tocker = 0,
    parser
};
const char* plugin_type_name(const plugin_type& t) {
    switch (t) {
        case plugin_type::tocker: return "tocker";
        case plugin_type::parser: return "parser";
        default: return "unknown/unsupported";
    }
}
using get_plugin_name_t = const char*(*)();
using get_plugin_version_t = const char*(*)();
using get_plugin_type_t = unsigned int(*)();
using tocker_ctor_t = tocker_t*(*)(const std::string&, const ntta_t&);
using parser_func_t = ntta_t*(*)(const std::vector<std::string>&, const std::vector<std::string>&);
using plugin_function_t = std::variant<tocker_ctor_t, parser_func_t>;
struct plugin_t {
    plugin_type type;
    std::string version;
    plugin_function_t function;
};

using plugin_map_t = std::map<std::string, plugin_t>;
std::ostream& operator<<(std::ostream&, const plugin_map_t&);

namespace plugins {
    plugin_map_t load(const std::vector<std::string> &search_directories);
}

#endif //AALTITOAD_PLUGIN_SYSTEM_H
