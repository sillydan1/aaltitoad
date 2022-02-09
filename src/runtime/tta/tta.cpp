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
#include "tta.h"
#include "extensions/overload"
#include "extensions/cparse_extensions.h"
#include "tinytimer/Timer.hpp"
#include "verifier/trace_output/TTAResugarizer.h"

TTA::StateChange operator+(TTA::StateChange a, TTA::StateChange b) {
    // Merge a and b
    a.symbols.map().merge(b.symbols.map());
    a.componentLocations.merge(b.componentLocations);
    return a;
}

TTA operator<<(const TTA& aa, const TTA::StateChange& b) {
    TTA a{};
    a.components = aa.components;
    a.InsertInternalSymbols(aa.symbols);
    a.InsertExternalSymbols(aa.GetExternalSymbols());
    bool changedSuccessfully = a.SetCurrentState(b);
    if(!changedSuccessfully) spdlog::critical("Something went wrong when trying to apply a statechange.");
    return a;
}

TTASymbol_t TTASymbolValueFromTypeAndValueStrings(const std::string& typestr, const std::string& valuestr) {
    return PopulateValueFromString(TTASymbolTypeFromString(typestr), valuestr);
}

TTASymbol_t TTASymbolTypeFromString(const std::string& typestr) {
    TTASymbol_t value;
    if     (typestr == "int") value = (int)0;
    else if(typestr == "long") value = (long)0;
    else if(typestr == "float") value = (float)0;
    else if(typestr == "bool") value = (bool)false;
    else if(typestr == "string") value = (std::string)"";
    else spdlog::error("Variable type '{0}' is not supported", typestr);
    // Value string
    return value;
}

TTASymbol_t PopulateValueFromString(const TTASymbol_t& type, const std::string& valuestr) {
    TTASymbol_t value = type;
    std::visit(overload(
            [&valuestr, &value](const float&      ){ value = std::stof(valuestr); },
            [&valuestr, &value](const int&        ){ value = std::stoi(valuestr); },
            [&valuestr, &value](const long&       ){ value = std::stol(valuestr); },
            [&valuestr, &value](const bool&       ){
                if(valuestr == "false") value = false;
                else if(valuestr == "true") value = true;
                else spdlog::error("Value '{0}' is not of boolean type", valuestr); // TODO: true/false is case-sensitive. Is this good?
            },
            [&valuestr, &value](const TTATimerSymbol&) { value = TTATimerSymbol{std::stof(valuestr)}; },
            [&valuestr, &value](const std::string&){
                if(valuestr[0] == '\"' && valuestr[valuestr.size()-1] == '\"')
                    value = valuestr.substr(1,valuestr.size()-2);
                else {
                    spdlog::warn("Missing '\"' on string value '{0}' - Adding them manually, but this should be corrected", valuestr);
                    value = valuestr;
                }
            }
    ), value);
    return value;
}

// TODO: This can be optimized by caching hashes on the state changes only. And there's no need to copy the symbols.
std::size_t TTA::GetCurrentStateHash() const {
    std::size_t state_hash = 0;
    for(auto& component : components)
        state_hash == 0 ?
        [&state_hash, &component](){ state_hash = std::hash<std::string>{}(component.second.currentLocation.identifier);}() :
        hash_combine(state_hash, component.second.currentLocation.identifier);

    for(auto& symbol : symbols.map()) {
        auto symbol_hash = std::hash<std::string>{}(symbol.first);   // hash of the symbol identifier
        // Combine with the symbol value
        switch(symbol.second->type) {
            case INT:   hash_combine(symbol_hash, symbol.second.asInt()    * COMBINE_MAGIC_NUM); break;
            case BOOL:  hash_combine(symbol_hash, symbol.second.asBool()   * COMBINE_MAGIC_NUM); break;
            case REAL:  hash_combine(symbol_hash, symbol.second.asDouble() * COMBINE_MAGIC_NUM); break;
            case STR:   hash_combine(symbol_hash, symbol.second.asString()); break;
            case TIMER: hash_combine(symbol_hash, symbol.second.asDouble() * COMBINE_MAGIC_NUM); break;
            default: spdlog::error("Symbol type '{0}' is not supported!", tokenTypeToString(symbol.second->type)); break;
        }
        hash_combine(state_hash, symbol_hash * COMBINE_MAGIC_NUM); // Combine with the overall state
    }
    return state_hash;
}

TTA::ComponentLocationMap TTA::GetCurrentLocations() const {
    // TODO: Maybe this should be cached
    ComponentLocationMap componentLocations = {};
    for(auto& component : components) componentLocations[component.first] = component.second.currentLocation;
    return componentLocations;
}

std::vector<std::string> TTA::GetCurrentLocationsLocationsOnly() const {
    std::vector<std::string> componentLocations = {};
    for(auto& component : components)
        componentLocations.push_back(component.second.currentLocation.identifier);
    return componentLocations;
}

bool TTA::SetCurrentState(const StateChange& newstate) {
    bool result = SetComponentLocations(newstate.componentLocations);
    result &= SetSymbols(newstate.symbols);
    tickCount++;
    return result;
}

bool TTA::SetComponentLocations(const ComponentLocationMap &locationChange) {
    for(auto& componentLocation : locationChange) {
        auto compit = components.find(componentLocation.first);
        if(compit != components.end())
            compit->second.currentLocation = componentLocation.second;
        else {
            spdlog::critical("Attempted to change the state of TTA failed. Component '{0}' does not exist.", componentLocation.first);
            return false;
        }
    }
    return true;
}

bool TTA::SetSymbols(const SymbolMap &symbolChange) {
    for(auto& symbol : symbolChange.map()) {
        auto symbolit = symbols.map().find(symbol.first);
        bool noerror = TypeCheck(symbol, symbolit);
        if(noerror) {
            symbols.map()[symbol.first] = symbol.second;
            if(externalSymbols.find(symbol.first) != externalSymbols.end())
                externalSymbols[symbol.first] = symbols.find(symbol.first);
        }
        else return false;
    }
    return true;
}

bool TTA::TypeCheck(const std::pair<const std::string, packToken> &symbol,
                    const std::map<std::string, packToken>::iterator &changingSymbol) const {
    auto x = symbol.second->type;
    auto y = changingSymbol->second->type;
    if(changingSymbol == symbols.map().end()) {
        spdlog::critical("Attempted to change the state of TTA failed. Symbol '{0}' does not exist.", symbol.first);
        return false;
    } else if(!(NUM & x & y) && !(x == VAR && (NUM & y))) {
        auto a = tokenTypeToString(changingSymbol->second->type);
        auto b = tokenTypeToString(symbol.second->type);
        spdlog::critical(
                "Attempted to change the state of TTA failed. Symbol '{0}' does not have the correct type. ({1} vs {2} (a := b))",
                symbol.first, a, b);
        return false;
    }
    return true;
}

bool TTA::IsCurrentStateImmediate() const {
    return std::any_of(components.begin(), components.end(), [](const auto& c){ return c.second.currentLocation.isImmediate; });
}

std::optional<StateMultiChoice> TTA::GetChangesFromEdge(const TTA::Edge& choice, bool& outInfluenceOverlap, std::map<std::string, std::vector<std::pair<std::string,std::string>>>& overlappingComponents) const {
    StateMultiChoice changes{};
    bool DoesUpdateInfluenceOverlap = AccumulateUpdateInfluences(choice, changes.symbolsToChange, overlappingComponents);
    outInfluenceOverlap |= DoesUpdateInfluenceOverlap;
    if(!CLIConfig::getInstance()["ignore-update-influence"] && DoesUpdateInfluenceOverlap) return{}; // No changes
    for(auto& symbolChange : changes.symbolsToChange) changes.symbolChanges.push_back(symbolChange.second);
    return changes;
}

void TTA::WarnAboutComponentOverlap(std::map<std::string, std::vector<std::pair<std::string,std::string>>> &overlappingComponents) const {
    spdlog::debug("Overlapping Components: (Tick#: {0})", tickCount);
    for (auto& componentCollection : overlappingComponents) {
        if (componentCollection.second.size() > 1) {
            for (auto& compname : componentCollection.second)
                spdlog::debug("{0}", compname.first);
        }
    }
}

TokenMap TTA::GetSymbolChangesAsMap(std::vector<UpdateExpression> &symbolChanges) const {
    SymbolMap symbolsCopy{};
    for(auto& symbolChange : symbolChanges) {
        if(symbols.map()[symbolChange.lhs]->type == TIMER)
            symbolsCopy[symbolChange.lhs] = packToken(symbolChange.Evaluate(symbols).asDouble(), PACK_IS_TIMER);
        else
            symbolsCopy[symbolChange.lhs] = symbolChange.Evaluate(symbols);
    }
    return symbolsCopy;
}

void TTA::ApplyComponentLocation(TTA::ComponentLocationMap &currentLocations,
                                 const std::pair<const std::string, TTA::Component> &component,
                                 TTA::Edge &pickedEdge) {// If we transition to the end location, immediately change to first location
    if(component.second.endLocation.identifier == pickedEdge.targetLocation.identifier)
        currentLocations[component.first] = component.second.initialLocation;
    else
        currentLocations[component.first] = pickedEdge.targetLocation;
}

TTA::StateChange TTA::GetNextTickState(const nondeterminism_strategy_t& strategy) const {
    ExpressionComponentMap overlappingComponents{}; // expr.lhs -> componentIdentifiers mapping
    bool updateInfluenceOverlapGlobal = false;
    StateMultiChoice sharedChanges{};
    for(auto& component : components) {
        auto enabledEdges = component.second.GetEnabledEdges(symbols); // TODO: Take Interesting variables into account
        if(enabledEdges.empty()) continue;
        auto &pickedEdge = PickEdge(enabledEdges, strategy);
        auto changes = GetChangesFromEdge(pickedEdge, updateInfluenceOverlapGlobal, overlappingComponents);
        if (changes.has_value()) sharedChanges.Merge(changes.value());
        ApplyComponentLocation(sharedChanges.currentLocations, component, pickedEdge);
    }
    if(updateInfluenceOverlapGlobal) WarnAboutComponentOverlap(overlappingComponents);
    auto symbolsCopy = GetSymbolChangesAsMap(sharedChanges.symbolChanges);
    return { sharedChanges.currentLocations, symbolsCopy };
}

std::vector<TTA::StateChange> TTA::GetNextTickStates() const {
    // Result type: [0] is shared, [>0] are choice changes
    StateMultiChoice sharedChanges{};
    std::vector<StateMultiChoice> choiceChanges{};
    ExpressionComponentMap overlappingComponents{}; // expr.lhs -> componentIdentifiers mapping
    bool updateInfluenceOverlapGlobal = false;
    for(auto& component : components) {
        auto enabledEdges = component.second.GetEnabledEdges(symbols); // TODO: Take Interesting variables into account
        if(enabledEdges.empty()) continue;
        bool hasNondeterminism = WarnIfNondeterminism(enabledEdges, component.first);
        for(auto& edge : enabledEdges) {
            auto changes = GetChangesFromEdge(edge, updateInfluenceOverlapGlobal, overlappingComponents);
            if(changes.has_value()) {
                if(hasNondeterminism) {
                    sharedChanges.Merge(changes.value());
                    ApplyComponentLocation(sharedChanges.currentLocations, component, edge);
                } else {
                    ApplyComponentLocation(changes->currentLocations, component, edge);
                    choiceChanges.push_back(changes.value());
                }
            }
        }
    }
    if(updateInfluenceOverlapGlobal) WarnAboutComponentOverlap(overlappingComponents);
    auto symbolsCopy = GetSymbolChangesAsMap(sharedChanges.symbolChanges);
    std::vector<TTA::StateChange> retVal{{ sharedChanges.currentLocations, symbolsCopy }};
    for(auto& change : choiceChanges)
        retVal.push_back({change.currentLocations, GetSymbolChangesAsMap(change.symbolChanges)});
    return retVal;
}

bool TTA::WarnIfNondeterminism(const std::vector<Edge>& edges, const std::string& componentName) {
    // If we aren't simulating, we should not warn about nondeterminism
    if(!CLIConfig::getInstance()["trace"]) return false;
    if(edges.size() > 1) {
        spdlog::warn("Non-deterministic choice in Component '{0}'.", TTAResugarizer::Resugar(componentName));
        spdlog::debug("Enabled edges in component '{0}':", TTAResugarizer::Resugar(componentName));
        for(auto& e : edges)
            spdlog::debug("{0} --> {1}",
                          TTAResugarizer::Resugar(e.sourceLocation.identifier),
                          TTAResugarizer::Resugar(e.targetLocation.identifier));
        spdlog::debug("----- / -----");
        return true;
    }
    return false;
}

std::string TTA::GetCurrentStateString() const {
    std::stringstream ss{}; ss << "{";
    for(auto& component : components) ss<<"\""<<component.first<<"\""<<": "<<"\""<<component.second.currentLocation.identifier<<"\",";
    for(auto& symbol : symbols.map()) ss<<"\""<<symbol.first<<"\""<<": "<<"\""<<symbol.second.str()<<"\",";
    ss << R"("OBJECT_END":"true"})"; // This is just a bad way of ending a json object. 
    return ss.str();
}

std::vector<TTA::Edge> TTA::Component::GetEnabledEdges(const SymbolMap& symbolMap) const {
    auto edges_from_current_state = edges.equal_range(currentLocation.identifier);
    std::vector<TTA::Edge> ret{};
    for(auto edge = edges_from_current_state.first; edge != edges_from_current_state.second; ++edge) {
        if(edge->second.guardExpression.empty()) {
            ret.push_back(edge->second);
            continue; // Empty guards are considered to be always satisfied
        }
        auto res = calculator::calculate(edge->second.guardExpression.c_str(), symbolMap);
        if(res.asBool()) ret.push_back(edge->second);
    }
    return ret;
}

void TTA::Tick(const nondeterminism_strategy_t& nondeterminismStrategy) {
    Timer<int> timer;
    timer.start();
    SetCurrentState(GetNextTickState());
    spdlog::info("Tick {0} time elapsed: {1} ms - (With printing and everything)", tickCount, timer.milliseconds_elapsed());
    tickCount++;
}

void TTA::InsertExternalSymbols(const TTA::SymbolMap& externalSymbolKeys) {
    symbols.map().insert(externalSymbolKeys.map().begin(), externalSymbolKeys.map().end());
    for(auto& elem : externalSymbolKeys.map())
        externalSymbols[elem.first] = symbols.find(elem.first);
}

void TTA::InsertInternalSymbols(const TTA::SymbolMap &internalSymbols) const {
    GetSymbols().map().insert(internalSymbols.map().begin(), internalSymbols.map().end());
}

std::optional<const TTA::Component*> TTA::GetComponent(const std::string &componentName) const {
    auto it = components.find(componentName);
    if(it == components.end()) return {};
    return { &it->second };
}

TTA::TTA() {
    // TODO: This is a stupid hack. Please implement an extension for cparse, so that the "False" literal is not case sensitive.
    // TODO: This hack makes it possible to assign "false := true", which is insanity incarnate.
    symbols.map()["false"] = false;
    symbols.map()["true"] = true;
}

TTA::Edge& TTA::PickEdge(std::vector<TTA::Edge>& edges, const nondeterminism_strategy_t &strategy) const {
    if(edges.empty()) throw std::logic_error("Trying to pick an edge from an empty list of edges is impossible.");
    if(edges.size() == 1) return edges[0];
    switch (strategy) {
        case nondeterminism_strategy_t::PICK_FIRST:
            return edges[0];
        case nondeterminism_strategy_t::PICK_LAST:
            return edges[edges.size()-1];
        case nondeterminism_strategy_t::PICK_RANDOM:
            return edges[rand() % edges.size()];
        default:
        case nondeterminism_strategy_t::PANIC:
            spdlog::critical("Panicking due to nondeterministic choice!");
            throw std::exception();
    }
}

void TTA::DelayAllTimers(double delayDelta) {
    for(auto& symbol : symbols.map()) {
        if(symbol.second->type == TIMER)
            symbols[symbol.first] = packToken(static_cast<double>(symbol.second.asDouble() + delayDelta), PACK_IS_TIMER);
    }
}

[[maybe_unused]] void TTA::SetAllTimers(double exactTime) {
    for(auto& symbol : symbols.map()) {
        if(symbol.second->type == TIMER)
            symbols[symbol.first] = packToken(exactTime, PACK_IS_TIMER);
    }
}

void TTA::StateChange::DelayTimerSymbols(SymbolMap& symbols, float delayDelta) {
    for(auto& symbol : symbols.map()) {
        if(symbol.second->type == TIMER)
            symbols[symbol.first] = packToken(static_cast<double>(symbol.second.asDouble() + delayDelta), PACK_IS_TIMER);
    }
}

bool AreRightHandSidesIndempotent(const std::map<std::string, std::vector<std::pair<std::string,std::string>>>& overlappingComponents, const std::string& key, const TTA::SymbolMap& symbols) {
    auto xx = overlappingComponents.find(key);
    bool indempotence = true;
    auto origval = calculator::calculate(xx->second.begin()->second.c_str(), symbols);
    for(auto& x : xx->second) {
        auto val = calculator::calculate(x.second.c_str(), symbols);
        if(origval != val)
            return false;
    }
    return indempotence;
}

bool TTA::AccumulateUpdateInfluences(const TTA::Edge& pickedEdge, std::multimap<std::string, UpdateExpression>& symbolsToChange, std::map<std::string, std::vector<std::pair<std::string,std::string>>>& overlappingComponents) const {
    bool updateInfluenceOverlap = false;
    for(auto& expr : pickedEdge.updateExpressions) {
        overlappingComponents[expr.lhs].push_back(std::make_pair(TTAResugarizer::Resugar(
                expr.lhs + " : " +
                pickedEdge.sourceLocation.identifier + " --> " +
                pickedEdge.targetLocation.identifier), expr.rhs));
        if(overlappingComponents.count(expr.lhs) > 0) {
            if(AreRightHandSidesIndempotent(overlappingComponents, expr.lhs, symbols)) {
                symbolsToChange.insert({expr.lhs, expr});
                continue;
            }
            spdlog::debug("Non-indempotent overlapping update influence on evaluation of update on edge {0} --> {1}. "
                         "Variable '{2}' is already being written to in this tick! - For more info, run with higher verbosity",
                         TTAResugarizer::Resugar(pickedEdge.sourceLocation.identifier),
                         TTAResugarizer::Resugar(pickedEdge.targetLocation.identifier),
                         TTAResugarizer::Resugar(expr.lhs)); // TODO: Idempotent variable assignment
            updateInfluenceOverlap = true;
            continue;
        }
        else
            symbolsToChange.insert({ expr.lhs, expr });
    }
    return updateInfluenceOverlap;
}

bool TTA::IsDeadlocked() const {
    return std::all_of(components.begin(), components.end(), [&] (const auto& component) {
        auto enabledEdges = component.second.GetEnabledEdges(symbols);
        return enabledEdges.empty();
    });
}

bool TTA::IsSymbolExternal(const std::string &identifier) const {
    return externalSymbols.find(identifier) != externalSymbols.end();
}

std::vector<TTA::Edge> TTA::GetCurrentEdges() const {
    std::vector<Edge> edges{};
    for(auto& component : components) {
        auto currentEdgesInComponent = component.second.edges.equal_range(component.second.currentLocation.identifier);
        for(auto edge = currentEdgesInComponent.first; edge != currentEdgesInComponent.second; ++edge) {
            edges.push_back(edge->second);
        }
    }
    return edges;
}

void TTA::InsertExternalSymbols(const TTA::ExternalSymbolMap &externalSymbolKeys) {
    for(auto& elem : externalSymbolKeys) {
        externalSymbols[elem.first] = symbols.find(elem.first);
    }
}

bool TTA::operator==(const TTA& other) const {
    for(auto& component : components) {
        auto componentsAreInSameLocation = other.components.find(component.first)->second.currentLocation == component.second.currentLocation;
        if(!componentsAreInSameLocation)
            return false;
    }
    for(auto& symbol : symbols.map()) {
        auto symbolsAreEqual = symbol.second.operator==(other.symbols.find(symbol.first));
        if(!symbolsAreEqual)
            return false;
    }
    for(auto& externalSymbol : externalSymbols) {
        auto symbolsAreEqual = externalSymbol.second->operator==(other.externalSymbols.find(externalSymbol.first)->second);
        if(!symbolsAreEqual)
            return false;
    }
    return true;
}
