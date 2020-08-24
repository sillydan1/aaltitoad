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
#include <rapidjson/reader.h>
#include "TTAParser.h"
#include <fstream>
#include <sstream>
#include <model_parsers/json/RapidJsonReaderStringStreamWrapper.h>

TTAIR_t TTAParser::ParseToIntermediateRep(const std::string& filepath) {
    /// NOTE: Look at the "dep/rapidjson/example/simplereader" example for how to parse this.
    // Open the file as a stringstream if it exists
    std::ifstream file{filepath};
    if(file) {
        std::stringstream filestream{};
        filestream << file.rdbuf();
        RapidJsonReaderStringStreamWrapper wrapper{std::move(filestream)};
        // Initialize a rapidjson::Reader reader class
        MyHandler h{};
        rapidjson::Reader reader{};
        // Parse.
        reader.Parse(wrapper, h);
        // TODO: Do more...
        file.close();
    }
    // If doesnt exist, error
    return {}; // TODO: Implement the TTAJsonTypeHandler
}

TTA_t TTAParser::ConvertToModelType(const TTAIR_t &intermediateRep) {
    return {};
}

TTAJsonTypeHandler::TTAJsonTypeHandler()  {
    SetDefaultReturnValue(false); // Fail by default.
}
