/**
 * Copyright (C) 2020 Asger Gitz-Johansen

   This file is part of mave.

    mave is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    mave is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with mave.  If not, see <https://www.gnu.org/licenses/>.
 */
#include <mavepch.h>
#include "TTAParser.h"
#include <model_parsers/json/RapidJsonReaderStringStreamWrapper.h>
#include <rapidjson/document.h>
#include <spdlog/spdlog.h>

TTAIR_t TTAParser::ParseToIntermediateRep(const std::string &path) {
    // Some json files are important to ignore, so Ignore-list:
    std::vector<std::string> ignore_list = {
            "IgnoreMe",     // ignore all files that want to be ignored
            "ignoreme",     // ignore all files that want to be ignored
            "Queries.json", // Queries are not component- or symbol-files
            ".parts",       // ignore all parts files
    };
    // Find all the .json files in the filepath
    TTAIR_t ttair{}; // TODO: Symbols.
    for (const auto & entry : std::filesystem::directory_iterator(path)) {
        bool should_skip = std::any_of(ignore_list.begin(), ignore_list.end(),
                      [&entry] (auto& el) { return entry.path().generic_string().find(el) != std::string::npos; });
        if(should_skip) continue;
        auto parsedComponent = ParseComponent(entry.path().generic_string());
        if(parsedComponent)
            ttair.AddComponent(std::move(parsedComponent.value()));
    }
    // Structure the TTAIR_t element
    return ttair;
}

std::optional<TTAIR_t::Component> TTAParser::ParseComponent(const std::string &filepath) {
    std::ifstream file{filepath};
    if(file) {
        auto dom_document = ParseDocumentDOMStyle(file);
        // Ensure existence of required high-level members
        if(IsDocumentAProperTTA(dom_document)) {
            return std::optional(TTAIR_t::Component{
                    .initialLocation = dom_document["initial_location"]["id"].GetString(),
                    .endLocation     = dom_document["final_location"]["id"].GetString(),
                    .edges = ParseEdges(dom_document["edges"]),
                    .isMain = dom_document["main"].GetBool()
            });
        } else
            spdlog::error("File '{0}' is improper", filepath);
        file.close();
        return {};
    }
    spdlog::error("File '{0}' does not exist", filepath);
    return {};
}

std::vector<TTAIR_t::Edge> TTAParser::ParseEdges(const rapidjson::Document::ValueType &edgeList) {
    std::vector<TTAIR_t::Edge> edges{};
    for(auto edge = edgeList.Begin(); edge != edgeList.End(); edge++)
        edges.push_back(ParseEdge(*edge));
    return edges;
}

TTAIR_t::Edge TTAParser::ParseEdge(const rapidjson::Document::ValueType &edge) {
    return {edge["source_location"].GetString(),
            edge["target_location"].GetString(),
            edge["guard"].GetString(),
            edge["update"].GetString()};
}

TTA_t TTAParser::ConvertToModelType(const TTAIR_t &intermediateRep) {
    spdlog::critical("ConvertToModelType is not implemented yet!");
    return {}; // TODO: Implement this!
}

rapidjson::Document TTAParser::ParseDocumentDOMStyle(const std::ifstream &file) {
    // Parse the file (DOM)
    std::stringstream filestream{};
    filestream << file.rdbuf();
    rapidjson::Document d;
    d.Parse(filestream.str().c_str());
    return d;
}

bool TTAParser::IsDocumentAProperTTA(const rapidjson::Document &document) {
    return  document.IsObject() &&
            DoesMemberExistAndIsObject(document, "initial_location") &&
            DoesMemberExistAndIsString(document["initial_location"], "id") &&
            DoesMemberExistAndIsObject(document, "final_location") &&
            DoesMemberExistAndIsString(document["final_location"], "id") &&
            DoesMemberExistAndIsArray( document, "edges") &&
            DoesMemberExistAndIsBool(  document, "main");
}

bool TTAParser::DoesMemberExistAndIsObject(const rapidjson::Document::ValueType &document, const std::string &membername) {
    auto memberIterator = document.FindMember(membername.c_str());
    return memberIterator != document.MemberEnd() && memberIterator->value.IsObject();
}

bool TTAParser::DoesMemberExistAndIsArray(const rapidjson::Document::ValueType &document, const std::string &membername) {
    auto memberIterator = document.FindMember(membername.c_str());
    return memberIterator != document.MemberEnd() && memberIterator->value.IsArray();
}

bool TTAParser::DoesMemberExistAndIsBool(const rapidjson::Document::ValueType &document, const std::string &membername) {
    auto memberIterator = document.FindMember(membername.c_str());
    return memberIterator != document.MemberEnd() && memberIterator->value.IsBool();
}

bool TTAParser::DoesMemberExistAndIsString(const rapidjson::Document::ValueType &document, const std::string &membername) {
    auto memberIterator = document.FindMember(membername.c_str());
    return memberIterator != document.MemberEnd() && memberIterator->value.IsString();
}
