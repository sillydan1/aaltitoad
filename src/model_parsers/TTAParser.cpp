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

TTAIR_t TTAParser::ParseToIntermediateRep(const std::string& filepath) {
    std::ifstream file{filepath};
    if(file) {
        auto dom_document = ParseDocumentDOMStyle(file);
        // Ensure existence of required high-level members

        // Convert to TTAIR_t

        auto x = d.FindMember("edges");
        if(x == d.MemberEnd())
            std::cout << "No edges!" << std::endl;
        else if(x->value.IsArray()) {
            auto xx = ParseEdges(x->value);
        } else {
            std::cout << "Error!" << std::endl;
        }
        // TODO: Do more...
        file.close();
    }
    // If the file doesnt exist -> error
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
    return {};
}

rapidjson::Document TTAParser::ParseDocumentDOMStyle(const std::ifstream &file) {
    // Parse the file (DOM)
    std::stringstream filestream{};
    filestream << file.rdbuf();
    rapidjson::Document d;
    d.Parse(filestream.str().c_str());
    return d;
}
