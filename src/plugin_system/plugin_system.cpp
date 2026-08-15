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
#include <dlfcn.h>
#include <util/warnings.h>
#include "plugin_system.h"

namespace aaltitoad::plugins {
    template<typename T>
    T load_symbol(void* handle, const std::string& symbol_name) {
        dlerror(); // clear errors
        auto val = (T) dlsym(handle, symbol_name.c_str());
        auto* err = dlerror();
        if(!val || err) {
            dlclose(handle);
            throw std::logic_error("could not find "+symbol_name+" symbol: "+err);
        }
        return val;
    }

    plugin_map_t load(const std::vector<std::string>& plugin_dirs) {
        auto rpath = std::getenv("AALTITOAD_LIBPATH");
        std::vector<std::string> look_dirs = { ".", "lib", "plugins" };
        if(rpath)
            look_dirs.emplace_back(rpath);
        look_dirs.insert(look_dirs.end(), plugin_dirs.begin(), plugin_dirs.end());
        plugin_map_t loaded_plugins{};
        for(auto& directory : look_dirs) {
            if(!std::filesystem::exists(directory)) {
                spdlog::trace("does not exist: {0}", directory);
                continue;
            }
            spdlog::trace("searching for plugins in: {0}", directory);
            for(const auto& entry : std::filesystem::directory_iterator(directory)) {
                try {
                    if (!entry.is_regular_file())
                        continue;
                    spdlog::trace("attempting to load file '{0}' as a plugin", entry.path().filename().string());
                    std::string entry_name = entry.path().c_str();
                    auto* handle = dlopen(entry.path().c_str(), RTLD_LAZY);
                    if (!handle)
                        throw std::logic_error("could not load as a shared/dynamic library");
                    auto stem = std::string(load_symbol<get_plugin_name_t>(handle, "get_plugin_name")());
                    auto type = static_cast<plugin_type>(load_symbol<get_plugin_type_t>(handle, "get_plugin_type")());
                    auto version = std::string(load_symbol<get_plugin_version_t>(handle, "get_plugin_version")());
                    if(loaded_plugins.contains(stem))
                        throw std::logic_error("plugin with name '" + stem + "' is already loaded. All plugins must have unique names");
                    switch(type) {
                        case plugin_type::tocker: {
                            auto ctor = load_symbol<tocker_ctor_t>(handle, "create_tocker");
                            loaded_plugins.insert(std::make_pair(stem, plugin_t{type, version, ctor}));
                            break;
                        }
                        case plugin_type::parser: {
                            auto load = load_symbol<parser_ctor_t>(handle, "create_parser");
                            loaded_plugins.insert(std::make_pair<>(stem, plugin_t{type, version, load}));
                            break;
                        }
                    }
                    spdlog::debug("loaded plugin '{0}'", stem);
                } catch (std::exception &e) {
                    aaltitoad::warnings::warn(plugin_load_failed, "failed to load '"+entry.path().string()+"' as a plugin: "+e.what());
                }
            }
        }
        return loaded_plugins;
    }
}

std::ostream& operator<<(std::ostream& stream, const plugin_map_t& map) {
    for(auto& t : map)
        stream << "  - [" << t.first << "] " << t.second.version << " (" << plugin_type_name(t.second.type) << ")" << std::endl;
    return stream;
}
