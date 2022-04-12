#include "hawk-parser.h"

ntta_t* hawk_parser_t::parse_folders(const std::vector<std::string> &folder_paths, const std::vector<std::string> &ignore_list) {
    symbol_table_t symbol_table{};
    component_map_t components{};
    for(const auto& filepath : folder_paths) {
        for (const auto &entry: std::filesystem::directory_iterator(filepath)) {
            try {
                if (std::find(ignore_list.begin(), ignore_list.end(), entry.path().c_str()) != ignore_list.end())
                    continue;

            } catch (std::exception &e) {
                spdlog::error("Unable to parse json file {0}: {1}", entry.path().c_str(), e.what());
                throw e;
            }
        }
    }
    return new ntta_t(state_t{components, symbol_table});
}

extern "C" {
    const char *get_plugin_name() {
        return "hawk_parser";
    }

    const char* get_plugin_version() {
        return PLUGIN_VERSION;
    }

    unsigned int get_plugin_type() {
        return static_cast<unsigned int>(plugin_type::parser);
    }

    ntta_t* load(const std::vector<std::string> &folders, const std::vector<std::string> &ignore_list) {
        return hawk_parser_t::parse_folders(folders, ignore_list);
    }
}
