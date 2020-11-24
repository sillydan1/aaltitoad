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

std::vector<Query> CTLQueryParser::ParseQueriesFile(const std::string &filepath, const TTA& tta) {
    std::vector<Query> queries{};
    auto file = std::ifstream{filepath};
    if(file) {
        spdlog::debug("Parsing '{0}' as a Query JSON file.", filepath);
        auto document = JSONParser::ParseDocumentDOMStyle(file);
        if(IsDocumentProperQueryDocument(document)) {
            auto queryarray = document.GetArray();
            std::for_each(queryarray.begin(), queryarray.end(), [&queries, &tta](auto& q){ queries.push_back(ParseQuery(q, tta)); });
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

Query CTLQueryParser::ParseQuery(const rapidjson::Document::ValueType& document, const TTA& tta) {
    auto queryString = std::string(document["query"].GetString());
    return Query{ParseQuantifier(queryString), ParseCondition(queryString, tta)};
}

Quantifier CTLQueryParser::ParseQuantifier(std::string full_query) {
    unsigned int retVal;
    ltrim(full_query); // Remove any whitespace left of the string
    switch (full_query[0]) {
        case 'E': retVal = static_cast<unsigned int>(Quantifier::EX); break;
        case 'A': retVal = static_cast<unsigned int>(Quantifier::AX); break;
        default: spdlog::critical("CTL Query '{0}' is improper. Queries are required to start with either an E or an A", full_query);
            throw std::exception();
    }
    auto substr = full_query.substr(1);
    ltrim(substr);
    switch (substr[0]) {
        case 'X': retVal += 0; break;
        case 'F': retVal += 1; break;
        case 'G': retVal += 2; break;
        case 'U': retVal += 3; break;
        default: spdlog::critical("CTL Query '{0}' is improper. Queries are required to start with either EX,EF,EG,EU,AX,AF,AG or AU", full_query);
            throw std::exception();
    }
    return static_cast<Quantifier>(retVal);
}

Condition CTLQueryParser::ParseCondition(std::string full_query, const TTA& tta) {
    // Remove the quantifier
    ltrim(full_query);
    full_query = full_query.substr(1);
    ltrim(full_query);
    full_query = full_query.substr(1);
    trim(full_query);
    // Then parse the condition
    return ParseSubCondition(full_query, tta);
}

Condition CTLQueryParser::ParseSubCondition(const std::string &subquery, const TTA &tta) {
    Condition topCondition{subquery};
    // Split over ANDs and ORs
    // TODO: Doctor robotnic quote: NO! (Parentheses)
    auto xx = subquery.begin();
    auto ctl_ir = ParseParetheses(xx, subquery.end());
    auto andloc = containsString(subquery, "&&");
    if(andloc.has_value()) {
        return LogicCondition{
            ParseSubCondition(subquery.substr(0, andloc.value()), tta),
            Junction::AND,
            ParseSubCondition(subquery.substr(andloc.value()+2), tta),
        };
    }
    auto orloc = containsString(subquery, "||");
    if(orloc.has_value()) {
        return LogicCondition{
                ParseSubCondition(subquery.substr(0, andloc.value()), tta),
                Junction::OR,
                ParseSubCondition(subquery.substr(andloc.value()), tta),
        };
    }

    if(subquery == "deadlock")
        return DeadlockCondition{};
    // Else, if query is a location


    return Condition{subquery};
}

CTLIR CTLQueryParser::ParseParetheses(std::string::const_iterator& iterator, std::string::const_iterator end) {
    // If there is a dot after the end-parenthesis, then it's just a paramenter parethesis.
    CTLIR current{};
    while(iterator != end) {
        if (*iterator == '(') {
            // Recurse
            auto ctl_ir = ParseParetheses(++iterator, end);
            if(iterator == end) { current.children.push_back(ctl_ir); return current; }
            if (*++iterator == '.') // Edge case e.g.: ComponentName(15, 0).LocationName should be a single token
                current.expression += ctl_ir.expression;
            else
                current.children.push_back(ctl_ir);
            current.expression += *iterator;
        }
        if (*iterator == ')') {
            return current;
        }
        if(iterator == end) break;
        current.expression += *iterator;
        ++iterator;
    }
    return current;
}
