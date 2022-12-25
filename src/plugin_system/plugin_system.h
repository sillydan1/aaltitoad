/**
 * aaltitoad - a verification engine for tick tock automata models
   Copyright (C) 2023 Asger Gitz-Johansen

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef AALTITOAD_PLUGIN_SYSTEM_H
#define AALTITOAD_PLUGIN_SYSTEM_H
#include <dlfcn.h>
#include <ntta/tta.h>

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
using tocker_ctor_t = aaltitoad::tocker_t*(*)(const std::string&, const aaltitoad::ntta_t&);
using parser_func_t = aaltitoad::ntta_t*(*)(const std::vector<std::string>&, const std::vector<std::string>&);
using plugin_function_t = std::variant<tocker_ctor_t, parser_func_t>;
struct plugin_t {
    plugin_type type;
    std::string version;
    plugin_function_t function;
};

using plugin_map_t = std::map<std::string, plugin_t>;
std::ostream& operator<<(std::ostream&, const plugin_map_t&);

namespace aaltitoad::plugins {
    plugin_map_t load(const std::vector<std::string> &search_directories);
}

#endif //AALTITOAD_PLUGIN_SYSTEM_H
