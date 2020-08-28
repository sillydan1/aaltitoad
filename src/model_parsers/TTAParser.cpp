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
#include <extensions/stringextensions.h>
#include "TTAParser.h"
bool ShouldSkipEntry(const std::filesystem::__cxx11::directory_entry& entry);

TTAIR_t TTAParser::ParseToIntermediateRep(const std::string &path) {
    TTAIR_t ttair{}; // TODO: Symbols.
    for (const auto & entry : std::filesystem::directory_iterator(path)) {
        if(ShouldSkipEntry(entry)) continue;
        auto parsedComponent = ParseComponent(entry.path().generic_string());
        if(parsedComponent)
            ttair.AddComponent(std::move(parsedComponent.value()));
    }
    return ttair;
}

// Some json files are important to ignore, so Ignore-list:
std::vector<std::string> componentIgnoreList = { // NOLINT(cert-err58-cpp)
        "ignore",       // ignore all files that want to be ignored
        "Queries.json", // Queries are not component- or symbol-files
        ".parts",       // parts files are not components
};

bool ShouldSkipEntry(const std::filesystem::__cxx11::directory_entry& entry) {
    auto entrystr = entry.path().string();
    return  !entry.is_regular_file() ||
            std::any_of(componentIgnoreList.begin(), componentIgnoreList.end(),
                        [&entrystr] (auto& el) { return entrystr.find(el) != std::string::npos; });
}

std::optional<TTAIR_t::Component> TTAParser::ParseComponent(const std::string &filepath) {
    std::ifstream file{filepath};
    if(file) {
        spdlog::debug("Parsing file '{0}' as a JSON file", filepath);
        auto dom_document = ParseDocumentDOMStyle(file);
        // Ensure existence of required high-level members
        if(IsDocumentAProperTTA(dom_document)) {
            spdlog::debug("File '{0}' is a proper TTA", filepath);
            return std::optional(TTAIR_t::Component{
                    .initialLocation = dom_document["initial_location"]["id"].GetString(),
                    .endLocation     = dom_document["final_location"]["id"].GetString(),
                    .isMain = dom_document["main"].GetBool(),
                    .edges = ParseEdges(dom_document["edges"]),
                    .symbols = ParseSymbolDeclarations(dom_document)
            });
        } else
            spdlog::error("File '{0}' is improper", filepath);
        file.close();
        return {};
    }
    spdlog::error("File '{0}' does not exist", filepath);
    return {};
}

rapidjson::Document TTAParser::ParseDocumentDOMStyle(const std::ifstream &file) {
    // Parse the file (DOM)
    // TODO: DOM Style can be slow and rapidjson provides faster parsing strategies. Extend when it becomes a problem
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
            DoesMemberExistAndIsBool(  document, "main") &&
            DoesMemberExistAndIsString(document, "declarations");
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

std::vector<TTAIR_t::Symbol> TTAParser::ParseSymbolDeclarations(const rapidjson::Document &document) {
    std::vector<TTAIR_t::Symbol> symbols{};
    auto decls = document["declarations"].GetString();
    if(decls != std::string("")) {
        auto decllines = split(decls, '\n');
        for(auto& decl : decllines) {
            auto symbol = ParseSymbolDeclaration(decl);
            if(symbol) symbols.emplace_back(std::move(symbol.value()));
        }
    }
    return symbols;
}

std::optional<TTAIR_t::Symbol> TTAParser::ParseSymbolDeclaration(const std::string &declaration) {
    auto tokens = split(declaration, ' ');
    if(tokens.size() < 5) {
        spdlog::error("Variable declaration '{0}' is missing tokens - The format is "
                      "[<Access modifier><Space><Type><Space><Identifier><Space>:=<Space><Initial Value>]",
                      declaration);
        return {};
    }
    if(tokens[3] != ":=") {
        spdlog::error("Variable declaration '{0}' does not contain a ':=' token", declaration);
        return {};
    }
    auto value = TTASymbolValueFromTypeAndValueStrings(tokens[1], tokens[4]);
    auto identifier = tokens[2];
    return TTAIR_t::Symbol{.identifier=identifier,.value=value};
}

TTA_t TTAParser::ConvertToModelType(const TTAIR_t &intermediateRep) {
    spdlog::critical("ConvertToModelType is not implemented yet!");
    return {}; // TODO: Implement this!
}
