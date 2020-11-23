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

#include "CTLQueryParser.h"
#include "json_parsing/JSONParser.h"
#include <rapidjson/rapidjson.h>
#include <extensions/stringextensions.h>

std::vector<BaseQuery> CTLQueryParser::ParseQueriesFile(const std::string &filepath) {
    std::vector<BaseQuery> queries{};
    auto file = std::ifstream{filepath};
    if(file) {
        spdlog::debug("Parsing '{0}' as a Query JSON file.", filepath);
        auto document = JSONParser::ParseDocumentDOMStyle(file);
        if(IsDocumentProperQueryDocument(document)) {
            auto queryarray = document.GetArray();
            std::for_each(queryarray.begin(), queryarray.end(), [&queries](auto& q){ queries.push_back(ParseQuery(q)); });
        } else {
            spdlog::critical("File '{0}' does not contain a proper query list", filepath);
            throw std::exception();
        }
    }
    return queries;
}

bool CTLQueryParser::IsDocumentProperQueryDocument(const rapidjson::Document::ValueType &document) {
    auto is_array = document.IsArray();
    if(is_array) {
        auto docarray = document.GetArray();
        bool accumulator = true;
        for(auto element = docarray.begin(); element != docarray.end() || !accumulator; ++element)
            accumulator = accumulator && IsElementProperQuery(*element);
        return accumulator;
    }
    return false;
}

bool CTLQueryParser::IsElementProperQuery(const rapidjson::Document::ValueType& document) {
    return  JSONParser::DoesMemberExistAndIsString(document, "query");
    // These are part of the specs, but they are technically not needed
            // JSONParser::DoesMemberExistAndIsString(document, "comment") &&
            // JSONParser::DoesMemberExistAndIsBool(document, "is_periodic");
}

BaseQuery CTLQueryParser::ParseQuery(const rapidjson::Document::ValueType& document) {
    BaseQuery query{};
    auto queryString = std::string(document["query"].GetString());
    ltrim(queryString); // Remove any whitespace left of the string
    switch (queryString[0]) {
        case 'E':
            break;
        case 'A':
            break;
        default:
            spdlog::critical("CTL Query '{0}' is improper. Queries are required to start with either an E or an A", queryString);
            throw std::exception();
    }

    query.condition.expression =

    return BaseQuery();
}
