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
#include "TTATypes.h"
#include <extensions/overload>

std::optional<std::vector<TTAIR_t::Component>::const_iterator> TTAIR_t::FindMainComponent() const {
    for(auto component_itr = components.begin(); component_itr != components.end(); component_itr++) {
        if(component_itr->isMain)
            return component_itr;
    }
    return {};
}

void TTAIR_t::AddComponent(TTAIR_t::Component&& component) {
    components.emplace_back(component);
    if(component.isMain) {
        if(hasMainComponentBeenAdded)
            spdlog::warn("Multiple main components are being parsed");
        hasMainComponentBeenAdded = true;
    }
}

TTASymbolType TTASymbolValueFromTypeAndValueStrings(const std::string& typestr, const std::string& valuestr) {
    return PopulateValueFromString(TTASymbolTypeFromString(typestr), valuestr);
}

TTASymbolType TTASymbolTypeFromString(const std::string& typestr) {
    TTASymbolType value;
    if     (typestr == "int") value = (int)0;
    else if(typestr == "float") value = (float)0;
    else if(typestr == "bool") value = (bool)false;
    else if(typestr == "string") value = (std::string)"";
    else spdlog::error("Variable type '{0}' is not supported", typestr);
    // Value string
    return value;
}

TTASymbolType PopulateValueFromString(const TTASymbolType& type, const std::string& valuestr) {
    TTASymbolType value = type;
    std::visit(overload(
            [&valuestr, &value](const float&      ){ value = std::stof(valuestr); },
            [&valuestr, &value](const int&        ){ value = std::stoi(valuestr); },
            [&valuestr, &value](const bool&       ){  if(valuestr == "false") value = false;
            else if(valuestr == "true") value = true;
            else spdlog::error("Value '{0}' is not of boolean type", valuestr);
            },
            [&valuestr, &value](const std::string&){
                if(valuestr[0] == valuestr[valuestr.size()-1] == '\"')
                    value = valuestr.substr(1,valuestr.size()-2);
                else
                    spdlog::error("Missing '\"' on string value '{0}'", valuestr);
            }
    ), value);
    return value;
}
