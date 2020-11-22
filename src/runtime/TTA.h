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
#include <mavepch.h>
#include <extensions/hash_combine>
#include <shunting-yard.h>
#include "UpdateExpression.h"

struct TTATimerSymbol {
    float current_value;
};

using TTASymbol_t = std::variant<
        int,
        float,
        bool,
        TTATimerSymbol,
        std::string
>;

TTASymbol_t TTASymbolValueFromTypeAndValueStrings(const std::string& typestr, const std::string& valuestr);
TTASymbol_t TTASymbolTypeFromString(const std::string& typestr);
TTASymbol_t PopulateValueFromString(const TTASymbol_t& type, const std::string& valuestr);

enum class nondeterminism_strategy_t {
    PANIC = 0,
    PICK_FIRST = 1,
    PICK_LAST = 2,
    PICK_RANDOM = 3
};

/***
 * Tick Tock Automata datastructure
 */
struct TTA {
    using SymbolMap = TokenMap;
    struct Location {
        bool isImmediate;
        std::string identifier;
    };
    struct Edge {
        Location sourceLocation;
        Location targetLocation;
        std::string guardExpression;
        std::vector<UpdateExpression> updateExpressions;
    };
    struct Component {
        // TODO: I dont like storing full strings.
        Location initialLocation;
        Location endLocation;
        Location currentLocation;
        bool isMain = false;
        std::unordered_multimap<std::string, Edge> edges = {};

        std::vector<Edge> GetEnabledEdges(const SymbolMap& symbolMap) const;
    };
    using ComponentMap = std::unordered_map<std::string, Component>;
    using ComponentLocationMap = std::unordered_map<std::string, Location>;
    struct State {
        ComponentLocationMap componentLocations;
        SymbolMap symbols;
    };
    ComponentMap components = {};

private:
    SymbolMap symbols = {};
    // This only works because we don't add or remove symbols during runtime
    std::vector<packToken*> externalSymbols = {}; // Still. Beware of dangling pointers!
    unsigned int tickCount = 0;

public:
    TTA();
    const SymbolMap& GetSymbols() const { return symbols; } // TODO: Am I still allowed to edit the symbol values themselves?
    void InsertExternalSymbols(const TTA::SymbolMap& externalSymbolKeys);
    void InsertInternalSymbols(const TTA::SymbolMap &internalSymbols);
    static std::size_t GetStateHash(const State& state);
    std::size_t GetCurrentStateHash() const;
    State GetCurrentState() const;
    ComponentLocationMap GetCurrentLocations() const;
    std::string GetCurrentStateString() const;
    bool IsCurrentStateImmediate() const;
    bool SetCurrentState(const State& newstate);
    static bool IsStateImmediate(const State& state);
    std::vector<State> GetNextTickStates(const nondeterminism_strategy_t& strategy = nondeterminism_strategy_t::PANIC) const;
    void DelayAllTimers(double delayDelta);
    void SetAllTimers(double exactTime);
    std::optional<const Component*> GetComponent(const std::string& componentName) const;
    TTA::Edge& PickEdge(std::vector<TTA::Edge>& edges, const nondeterminism_strategy_t& strategy) const;

    void Tick(const nondeterminism_strategy_t& nondeterminismStrategy = nondeterminism_strategy_t::PANIC);
    void Tock();

    inline unsigned int GetTickCount() const { return tickCount; }
};

#endif //MAVE_TTA_H
