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
#include <spdlog/spdlog.h>

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
