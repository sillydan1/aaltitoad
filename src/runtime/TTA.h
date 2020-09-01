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
#ifndef MAVE_TTA_H
#define MAVE_TTA_H
#include <extensions/hash_combine>

using TTASymbolType = std::variant<
        int,
        float,
        bool,
        std::string
>;

TTASymbolType TTASymbolValueFromTypeAndValueStrings(const std::string& typestr, const std::string& valuestr);
TTASymbolType TTASymbolTypeFromString(const std::string& typestr);
TTASymbolType PopulateValueFromString(const TTASymbolType& type, const std::string& valuestr);

struct TTA {
    struct Location {
        std::string identifier;
        bool isImmediate;
    };
    struct Edge {
        Location sourceLocation;
        Location targetLocation;
        std::string guardExpression;
        std::string updateExpression;
    };
    struct Component {
        std::string initialLocationIdentifier;
        std::string endLocationIdentifier;
        // TODO: This should be a hash. - I dont like storing full strings.
        std::string currentLocationIdentifier = initialLocationIdentifier;
        bool isMain = false;
        std::unordered_map<std::string, Edge> edges = {};
    };
    using SymbolMap = std::unordered_map<std::string, TTASymbolType>;
    using ComponentMap = std::unordered_map<std::string, Component>;

    SymbolMap symbols = {};
    ComponentMap components = {};

    std::size_t GetCurrentStateHash() {
        std::size_t state = 0; // TODO: Combine the hashes that are already calculated for you in the unordered_map, duh.
        for(auto& component : components)
            state == 0 ?    [&state, &component](){ state = std::hash<std::string>{}(component.second.currentLocationIdentifier);}() :
                            hash_combine(state, component.second.currentLocationIdentifier);
        return state;
    }
};

#endif //MAVE_TTA_H
