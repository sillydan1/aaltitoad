#ifndef AALTITOAD_TOCKER_PLUGIN_SYSTEM_H
#define AALTITOAD_TOCKER_PLUGIN_SYSTEM_H
#include <aaltitoadpch.h>
#include "runtime/tocker.h"

class tocker_plugin_system {
public:
    using tocker_interface = std::pair<tocker_creator, tocker_deleter>;
    std::map<std::string, tocker_interface> loaded_tockers{};
    void load(const std::vector<std::string>& search_directories);
};

#endif //AALTITOAD_TOCKER_PLUGIN_SYSTEM_H
