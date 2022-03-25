#include "tocker_plugin_system.h"

void tocker_plugin_system::load(const std::vector<std::string> &search_directories) {
    for(auto& directory : search_directories) {
        if(!std::filesystem::exists(directory)) {
            spdlog::warn("{0} does not exist", directory);
            continue;
        }
        for(const auto& entry: std::filesystem::directory_iterator(directory)) {
            try {
                if(entry.is_regular_file()) {

                }
            } catch(std::exception& e) {
                spdlog::info("Unable to load '{0}' as a tocker: {1}", entry.path(), e.what());
            }
        }
    }
}
