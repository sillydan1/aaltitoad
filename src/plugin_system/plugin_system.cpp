/**
 * aaltitoad - a verification engine for tick tock automata models
   Copyright (C) 2026 Asger Gitz-Johansen

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
#include <set>
#include <util/warnings.h>
#include "plugin_system.h"

namespace aaltitoad::plugins {
    template<typename T>
    T load_symbol(void* handle, const std::string& symbol_name) {
        dlerror(); // clear errors
        T val = (T) dlsym(handle, symbol_name.c_str());
        auto* err = dlerror();
        if(!val || err) {
            dlclose(handle);
            throw std::logic_error("could not find "+symbol_name+" symbol: "+err);
        }
        return val;
    }

    // Load the file as a plugin, if it is one. Files that are not shared libraries, and shared libraries that do not
    // define get_plugin_name, are not plugins. Throws std::logic_error if the file is a plugin that cannot be loaded
    void load_plugin(const std::filesystem::path& file, plugin_map_t& loaded_plugins) {
        spdlog::trace("attempting to load file '{0}' as a plugin", file.filename().string());
        auto* handle = dlopen(file.c_str(), RTLD_LAZY);
        if (!handle) {
            auto* error = dlerror();
            spdlog::debug("not a plugin: '{0}' is not a shared/dynamic library: {1}", file.string(), error ? error : "");
            return;
        }
        if (!dlsym(handle, "get_plugin_name")) {
            spdlog::debug("not a plugin: '{0}' does not define get_plugin_name", file.string());
            dlclose(handle);
            return;
        }
        auto stem = std::string(load_symbol<get_plugin_name_t>(handle, "get_plugin_name")());
        auto type = static_cast<plugin_type>(load_symbol<get_plugin_type_t>(handle, "get_plugin_type")());
        auto version = std::string(load_symbol<get_plugin_version_t>(handle, "get_plugin_version")());
        if (loaded_plugins.contains(stem)) {
            dlclose(handle);
            throw std::logic_error("plugin with name '" + stem + "' is already loaded. All plugins must have unique names");
        }
        switch (type) {
            case plugin_type::tocker: {
                auto ctor = load_symbol<tocker_ctor_t>(handle, "create_tocker");
                loaded_plugins.insert(std::make_pair(stem, plugin_t{type, version, ctor}));
                break;
            }
            case plugin_type::parser: {
                auto load = load_symbol<parser_func_t>(handle, "load");
                loaded_plugins.insert(std::make_pair<>(stem, plugin_t{type, version, load}));
                break;
            }
        }
        spdlog::debug("loaded plugin '{0}'", stem);
    }

    plugin_map_t load(const std::vector<std::string> &search_directories) {
        plugin_map_t loaded_plugins{};
        // NOTE: a directory can be given several times (e.g. "lib" from the installation prefix, and the plugin
        //       directory of the installation), and its plugins would then already be loaded
        std::set<std::filesystem::path> searched{};
        for (auto &directory: search_directories) {
            if (!std::filesystem::exists(directory)) {
                spdlog::trace("does not exist: {0}", directory);
                continue;
            }
            if (!searched.insert(std::filesystem::canonical(directory)).second) {
                spdlog::trace("already searched: {0}", directory);
                continue;
            }
            spdlog::trace("searching for plugins in: {0}", directory);
            for (const auto &entry: std::filesystem::directory_iterator(directory)) {
                if (!entry.is_regular_file())
                    continue;
                try {
                    load_plugin(entry.path(), loaded_plugins);
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
