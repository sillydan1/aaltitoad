#ifndef AALTITOAD_TOCKER_PLUGIN_SYSTEM_H
#define AALTITOAD_TOCKER_PLUGIN_SYSTEM_H
#include <aaltitoadpch.h>
#include "runtime/tocker.h"
#include <runtime/ntta.h>

using tocker_creator = tocker_t*(*)(const std::string&, const ntta_t&);
using tocker_deleter = void(*)(tocker_t*);
using tocker_name = const char*(*)();
using tocker_interface = std::pair<tocker_creator, tocker_deleter>;
using tocker_map_t = std::map<std::string, tocker_interface>;
class tocker_plugin_system {
public:
    static tocker_map_t load(const std::vector<std::string>& search_directories);
};

#endif //AALTITOAD_TOCKER_PLUGIN_SYSTEM_H
