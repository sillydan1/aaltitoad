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

#ifndef MAVE_CTLQUERYPARSER_H
#define MAVE_CTLQUERYPARSER_H
#include "query/QueryTypes.h"
#include <rapidjson/document.h>

class CTLQueryParser {
public:
    static std::vector<BaseQuery> ParseQueriesFile(const std::string& filepath);
    static BaseQuery ParseQuery(const rapidjson::Document::ValueType& document);

private:
    static bool IsDocumentProperQueryDocument(const rapidjson::Document::ValueType& document);
    static bool IsElementProperQuery(const rapidjson::Document::ValueType& document);
};


#endif //MAVE_CTLQUERYPARSER_H
