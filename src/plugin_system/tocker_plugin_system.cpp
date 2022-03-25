#include <extensions/string_extensions.h>
#include <dlfcn.h>
#include "tocker_plugin_system.h"

tocker_map_t tocker_plugin_system::load(const std::vector<std::string> &search_directories) {
    tocker_map_t loaded_tockers{};
    for(auto& directory : search_directories) {
        if(!std::filesystem::exists(directory)) {
            spdlog::warn("{0} does not exist", directory);
            continue;
        }
        spdlog::trace("Searching '{0}' for tockers", directory);
        for(const auto& entry: std::filesystem::directory_iterator(directory)) {
            try {
                if(entry.is_regular_file()) {
                    if(contains(entry.path().filename(), "tocker")) {
                        spdlog::trace("Attempting to load file '{0}' with substring 'tocker' in it", entry.path().filename().string());
                        auto* handle = dlopen(entry.path().c_str(), RTLD_NOW);
                        if(!handle)
                            throw std::logic_error("Could not load as a shared/dynamic library");
                        auto stem = entry.path().stem().string();
                        if(stem.substr(0, 3) == "lib")
                            stem = stem.substr(3);
                        auto create_symbol_name  = "create_" + stem;
                        auto destroy_symbol_name = "destroy_" + stem;
                        auto ctor = (tocker_creator)dlsym(handle, create_symbol_name.c_str());
                        auto dtor = (tocker_deleter)dlsym(handle, destroy_symbol_name.c_str());
                        if(!ctor || !dtor)
                            throw std::logic_error("Create or destroy symbol could not be found");
                        loaded_tockers[stem] = std::make_pair<>(ctor,dtor);
                        spdlog::debug("Loaded tocker '{0}'", stem);
                    }
                }
            } catch(std::exception& e) {
                spdlog::info("Failed to load '{0}' as a tocker: {1}", entry.path().string(), e.what());
            }
        }
    }
    return loaded_tockers;
}
