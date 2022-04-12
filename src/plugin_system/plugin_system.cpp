#include "plugin_system.h"

namespace plugins {
    template<typename T>
    T load_symbol(void* handle, const std::string& symbol_name) {
        T val = (T) dlsym(handle, symbol_name.c_str());
        if(!val)
            throw std::logic_error("Could not find "+symbol_name+" symbol");
        return val;
    }

    bool is_dynamic_library(const std::string& filename) {
        return contains(filename, ".so")
               || contains(filename, ".dll")
               || contains(filename, ".dylib");
    }

    plugin_map_t load(const std::vector<std::string> &search_directories) {
        plugin_map_t loaded_plugins{};
        for (auto &directory: search_directories) {
            if (!std::filesystem::exists(directory)) {
                spdlog::trace("Does not exist: {0}", directory);
                continue;
            }
            spdlog::trace("Searching for plugins in: {0}", directory);
            for (const auto &entry: std::filesystem::directory_iterator(directory)) {
                try {
                    if (entry.is_regular_file()) {
                        if (is_dynamic_library(entry.path().filename())) {
                            spdlog::trace("Attempting to load file '{0}' as a plugin",
                                          entry.path().filename().string());
                            auto *handle = dlopen(entry.path().c_str(), RTLD_LAZY);
                            if (!handle)
                                throw std::logic_error("Could not load as a shared/dynamic library");
                            auto stem = std::string(load_symbol<get_plugin_name_t>(handle, "get_plugin_name")());
                            auto type = static_cast<plugin_type>(load_symbol<get_plugin_type_t>(handle,
                                                                                                "get_plugin_type")());
                            auto version = std::string(load_symbol<get_plugin_version_t>(handle, "get_plugin_version")());
                            if (loaded_plugins.contains(stem))
                                throw std::logic_error("Plugin with name '" + stem +
                                                       "' is already loaded. All plugins must have unique names");
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
                            spdlog::debug("Loaded plugin '{0}'", stem);
                        }
                    }
                } catch (std::exception &e) {
                    spdlog::warn("Failed to load '{0}' as a plugin: {1}", entry.path().string(), e.what());
                }
            }
        }
        return loaded_plugins;
    }
}
