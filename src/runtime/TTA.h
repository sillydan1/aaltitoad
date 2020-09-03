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
#include <shunting-yard.h>

using TTASymbol_t = std::variant<
        int,
        float,
        bool,
        std::string
>;

TTASymbol_t TTASymbolValueFromTypeAndValueStrings(const std::string& typestr, const std::string& valuestr);
TTASymbol_t TTASymbolTypeFromString(const std::string& typestr);
TTASymbol_t PopulateValueFromString(const TTASymbol_t& type, const std::string& valuestr);

/***
 * Tick Tock Automata datastructure
 */
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
    using SymbolMap = TokenMap;
    using ComponentMap = std::unordered_map<std::string, Component>;
    struct State {
        std::unordered_map<std::string, std::string> componentLocations;
        SymbolMap symbols;
    };

    SymbolMap symbols = {};
    ComponentMap components = {};

    std::size_t GetStateHash(const State& state) const;
    std::size_t GetCurrentStateHash() const;
    State GetCurrentState() const;
    bool SetCurrentState(const State& newstate);
    // If this vector contains more than one entry, that means nondeterministic choice
    std::vector<State> GetNextTickStates() const;

};

#endif //MAVE_TTA_H
