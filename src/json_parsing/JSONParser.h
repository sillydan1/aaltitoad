/**
 * Copyright (C) 2020 Asger Gitz-Johansen

   This file is part of aaltitoad.

    aaltitoad is free software: you can redistribute it and/or modify
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

#ifndef MAVE_JSONPARSER_H
#define MAVE_JSONPARSER_H
#include <rapidjson/rapidjson.h>
#include <rapidjson/document.h>

struct JSONParser {
    static bool DoesMemberExistAndIsObject(const rapidjson::Document::ValueType& document, const std::string& membername);
    static bool DoesMemberExistAndIsArray(const rapidjson::Document::ValueType& document, const std::string& membername);
    static bool DoesMemberExistAndIsBool(const rapidjson::Document::ValueType& document, const std::string& membername);
    static bool DoesMemberExistAndIsInt(const rapidjson::Document::ValueType& document, const std::string& membername);
    static bool DoesMemberExistAndIsFloat(const rapidjson::Document::ValueType& document, const std::string& membername);
    static bool DoesMemberExistAndIsString(const rapidjson::Document::ValueType& document, const std::string& membername);
    static rapidjson::Document ParseDocumentDOMStyle(const std::ifstream &file);
};


#endif //MAVE_JSONPARSER_H
