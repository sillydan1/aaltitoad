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
#ifndef MAVE_TTAPARSER_H
#define MAVE_TTAPARSER_H
#include <rapidjson/document.h>
#include "ModelParser.h"
#include "TTATypes.h"

/// This TTAParser parses TTA's modelled in the H-UPPAAL tool
class TTAParser : ModelParser<TTA, TTAIR_t> {
public:
    TTA ParseFromFilePath(const std::string& filepath) override {
        return ConvertToModelType(ParseToIntermediateRep(filepath));
    }
protected:
    TTA   ConvertToModelType(const TTAIR_t& intermediateRep) override;
    TTAIR_t ParseToIntermediateRep(const std::string& path) override;
    static std::optional<TTAIR_t::Component> ParseComponent(const std::string& filepath);
private:
    static std::vector<TTAIR_t::Edge> ParseEdges(const rapidjson::Document::ValueType& edgeList, const rapidjson::Document& document);
    static TTAIR_t::Edge ParseEdge(const rapidjson::Document::ValueType& edge, const rapidjson::Document& document);
    static std::optional<const rapidjson::Document::ValueType*> FindLocationWithName(const rapidjson::Document& document, const std::string& query_name);
    static rapidjson::Document ParseDocumentDOMStyle(const std::ifstream& file);
    static bool IsDocumentAProperTTA(const rapidjson::Document& document);
    static bool DoesMemberExistAndIsObject(const rapidjson::Document::ValueType& document, const std::string& membername);
    static bool DoesMemberExistAndIsArray(const rapidjson::Document::ValueType& document, const std::string& membername);
    static bool DoesMemberExistAndIsBool(const rapidjson::Document::ValueType& document, const std::string& membername);
    static bool DoesMemberExistAndIsString(const rapidjson::Document::ValueType& document, const std::string& membername);
    static bool IsProperLocationList(const rapidjson::Document::ConstArray &locationList);
    static std::vector<TTAIR_t::Symbol> ParseSymbolDeclarations(const rapidjson::Document& document);
    static std::optional<TTAIR_t::Symbol> ParseSymbolDeclaration(const std::string& declaration);
    static TTA::SymbolMap ConvertSymbolListToSymbolMap(const std::vector<TTAIR_t::Symbol>& symbolList);
    static std::unordered_map<std::string, TTA::Edge> ConvertEdgeListToEdgeMap(const std::vector<TTAIR_t::Edge>& edge);
};

#endif //MAVE_TTAPARSER_H
