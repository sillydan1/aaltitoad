/**
 * Copyright (C) 2020 Asger Gitz-Johansen

   This file is part of aaltitoad.

    aaltitoad is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    aaltitoad is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with aaltitoad.  If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef MAVE_TTA_H
#define MAVE_TTA_H
#include <aaltitoadpch.h>
#include <extensions/hash_combine>
#include <shunting-yard.h>
#include <ctlparser/include/Tree.hpp>
#include <ctlparser/include/types.h>
#include "UpdateExpression.h"
#include "VariablePredicate.h"
#include "TTASymbol.h"

enum class nondeterminism_strategy_t {
    PANIC = 0,
    PICK_FIRST = 1,
    PICK_LAST = 2,
    PICK_RANDOM = 3,
    VERIFICATION = 4 /// Used for the verification engine.
};
struct StateMultiChoice;
/***
 * Tick Tock Automata datastructure
 */
struct TTA {
    using SymbolMap = TokenMap;
    using ExternalSymbolMap = std::unordered_map<std::string, packToken*>;
    using GuardExpression = Tree<ASTNode>;
    using GuardCollection = std::vector<GuardExpression>;
    struct Location {
        bool isImmediate;
        std::string identifier;
    };
    struct Edge {
        Location sourceLocation;
        Location targetLocation;
        std::string guardExpression;
        GuardCollection externalGuardCollection;
        std::vector<UpdateExpression> updateExpressions;
        bool ContainsExternalChecks() const { return ! externalGuardCollection.empty(); }
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
    struct StateChange {
        ComponentLocationMap componentLocations;
        SymbolMap symbols;
    };
    using InterestingState = std::pair<StateChange, std::vector<VariablePredicate>>;
    using InterestingStateCollection = std::vector<InterestingState>;
    ComponentMap components = {};

private:
    SymbolMap symbols = {};
    // This only works because we don't add or remove symbols during runtime
    ExternalSymbolMap externalSymbols = {}; // Still. Beware of dangling pointers!
    unsigned int tickCount = 0;

public:
    // TODO: Simplify this fucking class
    TTA();
    const SymbolMap& GetSymbols() const { return symbols; }
    const ExternalSymbolMap& GetExternalSymbols() const { return externalSymbols; }
    bool IsSymbolExternal(const std::string& identifier) const;
    void InsertExternalSymbols(const TTA::SymbolMap& externalSymbolKeys);
    void InsertInternalSymbols(const TTA::SymbolMap &internalSymbols) const;
    static std::size_t GetStateHash(const StateChange& state);
    std::size_t GetCurrentStateHash() const;
    StateChange GetCurrentState() const;
    ComponentLocationMap GetCurrentLocations() const;
    std::string GetCurrentStateString() const;
    bool IsCurrentStateImmediate() const;
    bool SetCurrentState(const StateChange& newstate);
    bool SetSymbols(const SymbolMap& symbolChange);
    bool SetComponentLocations(const ComponentLocationMap& locationChange);
    static bool IsStateImmediate(const StateChange& state);
    std::vector<Edge> GetCurrentEdges() const;
    std::vector<StateChange> GetNextTickStates(const nondeterminism_strategy_t& strategy = nondeterminism_strategy_t::PANIC) const;
    static bool WarnIfNondeterminism(const std::vector<TTA::Edge>& edges, const std::string& componentName) ;
    bool AccumulateUpdateInfluences(const TTA::Edge& pickedEdge, std::multimap<std::string, UpdateExpression>& symbolsToChange, std::map<std::string, std::vector<std::string>>& overlappingComponents) const;
    bool IsDeadlocked() const;
    void DelayAllTimers(double delayDelta);
    void SetAllTimers(double exactTime);
    std::optional<const Component*> GetComponent(const std::string& componentName) const;
    TTA::Edge& PickEdge(std::vector<TTA::Edge>& edges, const nondeterminism_strategy_t& strategy) const;
    void Tick(const nondeterminism_strategy_t& nondeterminismStrategy = nondeterminism_strategy_t::PANIC);
    inline unsigned int GetTickCount() const { return tickCount; }
    std::optional<StateMultiChoice> GetChangesFromEdge(const TTA::Edge& choice, bool& outInfluenceOverlap, std::map<std::string, std::vector<std::string>>& overlappingComponents) const;
    static void ApplyComponentLocation(ComponentLocationMap &currentLocations, const std::pair<const std::string, TTA::Component> &component, Edge &pickedEdge);
    TokenMap GetSymbolChangesAsMap(std::vector<UpdateExpression> &symbolChanges) const;
    void WarnAboutComponentOverlap(std::map<std::string, std::vector<std::string>> &overlappingComponents) const;
    bool TypeCheck(const std::pair<const std::string, packToken> &symbol, const std::map<std::string, packToken>::iterator &changingSymbol) const;
};

struct StateMultiChoice {
    std::vector<UpdateExpression> symbolChanges{};
    std::multimap<std::string, UpdateExpression> symbolsToChange{};
    TTA::ComponentLocationMap currentLocations{};

    void Merge(StateMultiChoice& other) {
        symbolChanges.insert(symbolChanges.end(), other.symbolChanges.begin(), other.symbolChanges.end());
        symbolsToChange.merge(other.symbolsToChange);
    }
};

#endif //MAVE_TTA_H
