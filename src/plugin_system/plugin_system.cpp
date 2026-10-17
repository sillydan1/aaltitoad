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
#include <algorithm>
#include <cctype>
#include <set>
#include <spdlog/spdlog.h>
#include <util/warnings.h>
#include "plugin_system.h"
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace aaltitoad::plugins {
    namespace {
        // The shared libraries of the platform (dlopen, or LoadLibrary on windows)
#ifdef _WIN32
        using library_t = HMODULE;

        auto last_error() -> std::string {
            char* message = nullptr;
            auto length = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                                         nullptr, GetLastError(), 0, reinterpret_cast<char*>(&message), 0, nullptr);
            std::string result{message != nullptr ? std::string{message, length} : std::string{}};
            LocalFree(message);
            while(!result.empty() && (result.back() == '\n' || result.back() == '\r'))
                result.pop_back();
            return result;
        }

        // NOTE: only .dll files are libraries (LoadLibrary would add .dll to a name without an extension)
        auto open_library(const std::filesystem::path& file) -> library_t {
            auto extension = file.extension().string();
            std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c){ return std::tolower(c); });
            if(extension != ".dll") {
                SetLastError(ERROR_BAD_EXE_FORMAT);
                return nullptr;
            }
            // NOTE: files that are not libraries would otherwise make windows show an error dialog
            DWORD previous_mode = 0;
            SetThreadErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX, &previous_mode);
            auto* library = LoadLibraryW(file.c_str());
            SetThreadErrorMode(previous_mode, nullptr);
            return library;
        }

        auto find_symbol(library_t library, const std::string& name) -> void* {
            return reinterpret_cast<void*>(GetProcAddress(library, name.c_str()));
        }

        void close_library(library_t library) {
            FreeLibrary(library);
        }
#else
        using library_t = void*;

        auto last_error() -> std::string {
            auto* error = dlerror();
            return error != nullptr ? error : "";
        }

        auto open_library(const std::filesystem::path& file) -> library_t {
            return dlopen(file.c_str(), RTLD_LAZY);
        }

        auto find_symbol(library_t library, const std::string& name) -> void* {
            dlerror(); // clear errors
            return dlsym(library, name.c_str());
        }

        void close_library(library_t library) {
            dlclose(library);
        }
#endif
    }

    template<typename T>
    T load_symbol(library_t handle, const std::string& symbol_name) {
        auto* symbol = find_symbol(handle, symbol_name);
        if(!symbol) {
            auto error = last_error();
            close_library(handle);
            throw std::logic_error("could not find "+symbol_name+" symbol: "+error);
        }
        return reinterpret_cast<T>(symbol);
    }

    // Load the file as a plugin, if it is one. Files that are not shared libraries, and shared libraries that do not
    // define get_plugin_name, are not plugins. Throws std::logic_error if the file is a plugin that cannot be loaded
    void load_plugin(const std::filesystem::path& file, plugin_map_t& loaded_plugins) {
        spdlog::trace("attempting to load file '{0}' as a plugin", file.filename().string());
        auto handle = open_library(file);
        if (!handle) {
            spdlog::debug("not a plugin: '{0}' is not a shared/dynamic library: {1}", file.string(), last_error());
            return;
        }
        if (!find_symbol(handle, "get_plugin_name")) {
            spdlog::debug("not a plugin: '{0}' does not define get_plugin_name", file.string());
            close_library(handle);
            return;
        }
        auto stem = std::string(load_symbol<get_plugin_name_t>(handle, "get_plugin_name")());
        auto type = static_cast<plugin_type>(load_symbol<get_plugin_type_t>(handle, "get_plugin_type")());
        auto version = std::string(load_symbol<get_plugin_version_t>(handle, "get_plugin_version")());
        if (loaded_plugins.contains(stem)) {
            close_library(handle);
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

#ifdef _WIN32
    auto executable_directory() -> std::filesystem::path {
        std::wstring path(MAX_PATH, L'\0');
        while(true) {
            auto length = GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size()));
            if(length < path.size()) {
                path.resize(length);
                return std::filesystem::path{path}.parent_path();
            }
            path.resize(path.size() * 2);
        }
    }
#endif

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
