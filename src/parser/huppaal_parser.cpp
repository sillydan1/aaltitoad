#include "huppaal_parser.h"
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

namespace aaltitoad::huppaal {
    auto load(const std::vector<std::string>& filepaths, const std::vector<std::string> &ignore_list) -> aaltitoad::ntta_t* {
        aaltitoad::ntta_builder builder{};
        for(const auto& filepath : filepaths) {
            for(const auto &entry: std::filesystem::directory_iterator(filepath)) {
                try {
                    if(std::find(ignore_list.begin(), ignore_list.end(), entry.path().c_str()) != ignore_list.end())
                        continue;

                    std::ifstream input_filestream(entry.path());
                    auto json_file = nlohmann::json::parse(input_filestream);
                    builder.add_symbols(load_declarations(json_file, builder.symbols));
                    if(json_file.contains("name"))
                        builder.add_tta(json_file["name"], load_tta(json_file, builder.symbols).build());
                } catch (std::exception &e) {
                    spdlog::error("Unable to parse json file {0}: {1}", entry.path().c_str(), e.what());
                    throw e;
                }
            }
        }
        return builder.build_heap();
    }

    auto load_declarations(const nlohmann::json& json_file, const expr::symbol_table_t& symbols) -> expr::symbol_table_t {
        expr::symbol_table_t result{};
        expr::interpreter i{symbols};
        if(json_file.contains("declarations"))
            result += i.interpret_declarations(json_file["declarations"]);
        if(json_file.contains("parts"))
            for(auto& p : json_file["parts"])
                result += load_part(p);
        return result;
    }

    // TODO: Missing external/internal check
    auto load_part(const nlohmann::json& json_file) -> expr::symbol_table_t {
        expr::symbol_table_t result{{}};
        expr::symbol_value_t val{};
        val <<= json_file["SpecificType"]["Variable"]["Value"];
        result[json_file["PartName"]] = val;
        return result;
    }

    auto load_tta(const nlohmann::json& json_file, expr::symbol_table_t& symbols) -> aaltitoad::tta_builder {
        expr::symbol_table_t externals{}; // TODO: actual externals
        aaltitoad::tta_builder builder{symbols, externals};
        builder.set_starting_location(json_file["initial_location"]["id"]);
        for(auto& loc : json_file["locations"])
            builder.add_location(loc["id"]);
        // TODO: stitch "final_location" together with "initial_location"
        // TODO: recurse on sub_components (sequential/parallel composition)
        for(auto& edge : json_file["edges"])
            builder.add_edge({edge["source_location"], edge["target_location"],
                              (std::string)edge["guard"], (std::string)edge["update"]});
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
