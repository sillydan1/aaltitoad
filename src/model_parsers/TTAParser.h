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

#include <rapidjson/pointer.h>
#include "ModelParser.h"
#include "TTATypes.h"
#include "json/BaseJsonTypeHandler.h"

/// This TTAParser parses TTA's modelled in the H-UPPAAL tool
class TTAParser : ModelParser<TTA_t, TTAIR_t> {
public:
    TTA_t ParseFromFile(const std::string& filepath) override {
        return ConvertToModelType(ParseToIntermediateRep(filepath));
    }
protected:
    TTA_t   ConvertToModelType(const TTAIR_t& intermediateRep) override;
    TTAIR_t ParseToIntermediateRep(const std::string& path) override;
    std::optional<TTAIR_t::Component> ParseComponent(const std::string& filepath);
private:
    rapidjson::Document ParseDocumentDOMStyle(const std::ifstream& file);
    bool IsDocumentAProperTTA(const rapidjson::Document& document);
    std::vector<TTAIR_t::Edge> ParseEdges(const rapidjson::Document::ValueType& edgeList);
    TTAIR_t::Edge ParseEdge(const rapidjson::Document::ValueType& edge);
    bool DoesMemberExistAndIsObject(const rapidjson::Document &document, const std::string &membername);
    bool DoesMemberExistAndIsArray(const rapidjson::Document &document, const std::string &membername);
    bool DoesMemberExistAndIsBool(const rapidjson::Document &document, const std::string &membername);
};

#endif //MAVE_TTAPARSER_H
