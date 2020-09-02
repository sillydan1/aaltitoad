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
#include <extensions/overload>
#include "TTAParser.h"
bool ShouldSkipEntry(const std::filesystem::__cxx11::directory_entry& entry);

TTAIR_t TTAParser::ParseToIntermediateRep(const std::string &path) {
    TTAIR_t ttair{};
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
                    .name = GetFileNameOnly(filepath),
                    .initialLocation = dom_document["initial_location"]["id"].GetString(),
                    .endLocation     = dom_document["final_location"]["id"].GetString(),
                    .isMain = dom_document["main"].GetBool(),
                    .edges = ParseEdges(dom_document["edges"], dom_document),
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
            DoesMemberExistAndIsString(document, "declarations") &&
            DoesMemberExistAndIsArray(document, "locations") &&
            IsProperLocationList(document["locations"].GetArray());
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

bool TTAParser::IsProperLocationList(const rapidjson::Document::ConstArray &locationList) {
    bool accumulator = true;
    for(auto locationObject = locationList.begin(); locationObject != locationList.end() || !accumulator; locationObject++) {
        accumulator = accumulator && DoesMemberExistAndIsString(*locationObject, "id");
        accumulator = accumulator && DoesMemberExistAndIsString(*locationObject, "urgency");
    }
    return accumulator;
}

std::vector<TTAIR_t::Edge>
TTAParser::ParseEdges(const rapidjson::Document::ValueType &edgeList, const rapidjson::Document& document) {
    std::vector<TTAIR_t::Edge> edges{};
    for(auto edge = edgeList.Begin(); edge != edgeList.End(); edge++)
        edges.push_back(ParseEdge(*edge, document));
    return edges;
}

std::optional<const rapidjson::Document::ValueType*>
TTAParser::FindLocationWithName(const rapidjson::Document& document, const std::string& query_name) {
    if(document["initial_location"]["id"].GetString() == query_name)
        return { &document["initial_location"] };
    else if(document["final_location"]["id"].GetString() == query_name)
        return { &document["final_location"] };

    auto array = document["locations"].GetArray();
    for(int i = 0; i < array.Size(); i++) {
        auto id = array[i].FindMember("id");
        if(id->value.GetString() == query_name) return { &array[i] };
    }
    spdlog::error("Unable to find Location with 'id':'{0}' - Make sure that the location names are correct!", query_name);
    return {};
}

TTAIR_t::Edge TTAParser::ParseEdge(const rapidjson::Document::ValueType &edge, const rapidjson::Document& document) {
    auto sourceLocation = FindLocationWithName(document, edge["source_location"].GetString());
    auto targetLocation = FindLocationWithName(document, edge["target_location"].GetString());
    if(!sourceLocation) throw std::logic_error("Unable to locate source location for edge");
    if(!targetLocation) throw std::logic_error("Unable to locate target location for edge");
    return TTAIR_t::Edge{
            .sourceLocation = {
                .identifier = (*sourceLocation.value())["id"].GetString(),
                .isImmediate = (*sourceLocation.value())["urgency"].GetString() == std::string("urgent")
            },
             .targetLocation = {
                .identifier = (*targetLocation.value())["id"].GetString(),
                .isImmediate = (*targetLocation.value())["urgency"].GetString() == std::string("urgent")
            },
            .guardExpression = edge["guard"].GetString(),
            .updateExpression = edge["update"].GetString()};
}

std::vector<TTAIR_t::Symbol> TTAParser::ParseSymbolDeclarations(const rapidjson::Document &document) {
    std::vector<TTAIR_t::Symbol> symbols{};
    auto decls = document["declarations"].GetString();
    if(decls != std::string("")) {
        auto declarationLines = split(decls, '\n');
        for(auto& decl : declarationLines) {
            auto symbol = ParseSymbolDeclaration(decl);
            if(symbol) symbols.emplace_back(std::move(symbol.value()));
            // Improper symbol decls should result in fatal error
            else       throw std::logic_error("Symbol declaration failure");
        }
    }
    return symbols;
}

std::optional<TTAIR_t::Symbol> TTAParser::ParseSymbolDeclaration(const std::string &declaration) {
    auto tokens = split(declaration, ' ');
    if(tokens.size() < 5) {
        spdlog::critical("Variable declaration '{0}' is missing tokens - The format is "
                      "[<Access modifier><Space><Type><Space><Identifier><Space>:=<Space><Initial Value>]",
                      declaration);
        return {};
    }
    if(tokens[3] != ":=") {
        spdlog::critical("Variable declaration '{0}' does not contain a ':=' token", declaration);
        return {};
    }
    auto value = TTASymbolValueFromTypeAndValueStrings(tokens[1], tokens[4]);
    auto identifier = tokens[2];
    return TTAIR_t::Symbol{.identifier=identifier,.value=value};
}

TTA TTAParser::ConvertToModelType(const TTAIR_t &intermediateRep) {
    TTA tta{};
    for(auto& comp : intermediateRep.components) {
        tta.components[comp.name] = {
                .initialLocationIdentifier = comp.initialLocation,
                .endLocationIdentifier     = comp.endLocation,
                .currentLocationIdentifier = comp.initialLocation,
                .isMain                    = comp.isMain,
                .edges                     = ConvertEdgeListToEdgeMap(comp.edges),
        };
        auto componentSymbols = ConvertSymbolListToSymbolMap(comp.symbols);
        tta.symbols.map().insert(componentSymbols.map().begin(), componentSymbols.map().end());
    }
    return tta;
}

TTA::SymbolMap TTAParser::ConvertSymbolListToSymbolMap(const std::vector<TTAIR_t::Symbol> &symbolList) {
    TTA::SymbolMap map{};
    std::for_each(symbolList.begin(), symbolList.end(), [&map] (auto& symbol) {
        std::visit(overload(
                [&](const int& value)        { map[symbol.identifier] = value; },
                [&](const float& value)      { map[symbol.identifier] = value; },
                [&](const bool& value)       { map[symbol.identifier] = value; },
                [&](const std::string& value){ map[symbol.identifier] = value; }
                ), symbol.value);
    });
    return map;
}

std::unordered_map<std::string, TTA::Edge>
TTAParser::ConvertEdgeListToEdgeMap(const std::vector<TTAIR_t::Edge> &edgeList) {
    std::unordered_map<std::string, TTA::Edge> map{};
    for(auto& edge : edgeList) {
        map[edge.sourceLocation.identifier] = {
                .sourceLocation           = {edge.sourceLocation.identifier, edge.sourceLocation.isImmediate},
                .targetLocation           = {edge.targetLocation.identifier, edge.targetLocation.isImmediate},
                .guardExpression          = edge.guardExpression,   // TODO: Compile this as a thing
                .updateExpression         = edge.updateExpression  // TODO: Compile this as a thing
        };
    }
    return map;
}
