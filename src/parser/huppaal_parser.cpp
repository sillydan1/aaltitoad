#include "huppaal_parser.h"
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <ntta/builder/ntta_builder_2.h>

namespace aaltitoad::huppaal {
    auto load(const std::vector<std::string>& filepaths, const std::vector<std::string> &ignore_list) -> aaltitoad::ntta_t* {
        aaltitoad::ntta_builder2 builder{};
        for(const auto& filepath : filepaths) {
            for(const auto &entry: std::filesystem::directory_iterator(filepath)) {
                try {
                    if(std::find(ignore_list.begin(), ignore_list.end(), entry.path().c_str()) != ignore_list.end())
                        continue;

                    std::ifstream input_filestream(entry.path());
                    auto json_file = nlohmann::json::parse(input_filestream);
                    load_declarations(builder, json_file);
                    if(json_file.contains("name"))
                        builder.add_tta(load_tta(json_file));
                } catch (std::exception &e) {
                    spdlog::error("Unable to parse json file {0}: {1}", entry.path().c_str(), e.what());
                    throw e;
                }
            }
        }
        return builder.build().build_heap();
    }

    void load_declarations(ntta_builder2& b, const nlohmann::json& json_file) {
        if(json_file.contains("declarations"))
            b.add_declarations(json_file["declarations"]);
        if(json_file.contains("parts"))
            for(auto& p : json_file["parts"])
                // TODO: Missing external/internal check
                b.add_external_declarations(load_part(p));
    }

    auto load_part(const nlohmann::json& json_file) -> std::string {
        std::stringstream ss{};
        ss << json_file["PartName"] << " := " << json_file["SpecificType"]["Variable"]["Value"];
        return ss.str();
    }

    auto load_tta(const nlohmann::json& json_file) -> aaltitoad::tta_builder2 {
        aaltitoad::tta_builder2 builder{};
        builder.set_start_location(json_file["initial_location"]["id"]);
        for(auto& loc : json_file["locations"])
            builder.add_location(loc["id"]);
        for(auto& edge : json_file["edges"]) {
            auto target_loc = edge["target_location"];
            if(edge["target_location"] == json_file["final_location"]["id"])
                target_loc = builder.initial_location;
            builder.add_edge({edge["source_location"], target_loc,
                              (std::string) edge["guard"], (std::string) edge["update"]});
        }
        return builder;
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
