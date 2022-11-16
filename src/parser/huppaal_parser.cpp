#include "huppaal_parser.h"
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <ntta/builder/ntta_builder_2.h>
#include "scoped_template_builder/scoped_template_builder.h"

namespace aaltitoad::huppaal {
    auto load(const std::vector<std::string>& filepaths, const std::vector<std::string> &ignore_list) -> aaltitoad::ntta_t* {
        // TODO: use stl parallel constructs to load files faster
        scoped_template_builder builder{};
        for(const auto& filepath : filepaths) {
            for(const auto &entry: std::filesystem::directory_iterator(filepath)) {
                try {
                    if(should_ignore(entry, ignore_list)) {
                        spdlog::trace("ignoring file {0}", entry.path().c_str());
                        continue;
                    }

                    spdlog::trace("loading file {0}", entry.path().c_str());
                    std::ifstream input_filestream(entry.path());
                    auto json_file = nlohmann::json::parse(input_filestream);
                    if(json_file.contains("name")) {
                        spdlog::trace("file is a template");
                        builder.add_template(json_file.get<model::tta_template>());
                    } else {
                        spdlog::trace("file is a symbols declaration file");
                        builder.add_global_symbols(load_part(json_file));
                    }
                } catch (std::exception &e) {
                    spdlog::error("unable to parse json file {0}: {1}", entry.path().c_str(), e.what());
                    throw e;
                }
            }
        }
        return builder.build_heap();
    }

    auto should_ignore(const std::filesystem::directory_entry& entry, const std::vector<std::string>& ignore_list) -> bool {
        return std::any_of(ignore_list.begin(), ignore_list.end(),
                           [&entry](const std::string& ig){ return should_ignore(entry, ig); });
    }

    auto should_ignore(const std::filesystem::directory_entry& entry, const std::string& ignore_regex) -> bool {
        return std::regex_match(entry.path().c_str(), std::regex{ignore_regex});
    }

    auto load_part(const nlohmann::json& json_file) -> std::string {
        std::stringstream ss{};
        if(json_file["SpecificType"].contains("Variable"))
            ss << (std::string)json_file["PartName"] << " := " << json_file["SpecificType"]["Variable"]["Value"];
        else if(json_file["SpecificType"].contains("Timer"))
            ss << (std::string)json_file["PartName"] << " := " << json_file["SpecificType"]["Timer"]["Value"] << "_ms";
        else
            throw std::logic_error("invalid piece of json: "+to_string(json_file));
        return ss.str();
    }
}

extern "C" {
    const char* get_plugin_name() {
        return "huppaal_parser";
    }
    const char* get_plugin_version() {
        return "v1.0.0";
    }
    plugin_type get_plugin_type() {
        return plugin_type::parser;
    }
    aaltitoad::ntta_t* load(const std::vector<std::string>& folders, const std::vector<std::string>& ignore_list) {
        return aaltitoad::huppaal::load(folders, ignore_list);
    }
}
