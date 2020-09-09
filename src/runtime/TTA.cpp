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
#include "TTA.h"
#include <extensions/overload>

TTASymbol_t TTASymbolValueFromTypeAndValueStrings(const std::string& typestr, const std::string& valuestr) {
    return PopulateValueFromString(TTASymbolTypeFromString(typestr), valuestr);
}

TTASymbol_t TTASymbolTypeFromString(const std::string& typestr) {
    TTASymbol_t value;
    if     (typestr == "int") value = (int)0;
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
            [&valuestr, &value](const bool&       ){
                if(valuestr == "false") value = false;
                else if(valuestr == "true") value = true;
                else spdlog::error("Value '{0}' is not of boolean type", valuestr);
            },
            [&valuestr, &value](const std::string&){
                if(valuestr[0] == '\"' && valuestr[valuestr.size()-1] == '\"')
                    value = valuestr.substr(1,valuestr.size()-2);
                else
                    spdlog::error("Missing '\"' on string value '{0}'", valuestr);
            }
    ), value);
    return value;
}

// TODO: This can be optimized by caching hashes on the state changes only. And there's no need to copy the symbols.
std::size_t TTA::GetCurrentStateHash() const {
    return GetStateHash(GetCurrentState());
}

TTA::State TTA::GetCurrentState() const {
    return { .componentLocations = GetCurrentLocations(), .symbols = symbols };
}

TTA::ComponentLocationMap TTA::GetCurrentLocations() const {
    ComponentLocationMap componentLocations = {};
    for(auto& component : components) componentLocations[component.first] = component.second.currentLocation;
    return componentLocations;
}

std::size_t TTA::GetStateHash(const State& state) {
    std::size_t state_hash = 0;
    for(auto& component : state.componentLocations)
        state_hash == 0 ?
        [&state_hash, &component](){ state_hash = std::hash<std::string>{}(component.second.identifier);}() :
        hash_combine(state_hash, component.first);

    for(auto& symbol : state.symbols.map()) {
        auto symbol_hash = std::hash<std::string>{}(symbol.first);   // hash of the symbol identifier
        // Combine with the symbol value
        switch(symbol.second->type) {
            case INT:  hash_combine(symbol_hash, symbol.second.asInt());    break;
            case BOOL: hash_combine(symbol_hash, symbol.second.asBool());   break;
            case REAL: hash_combine(symbol_hash, symbol.second.asDouble()); break;
            case STR:  hash_combine(symbol_hash, symbol.second.asString()); break;
            default: spdlog::error("Symbol type '{0}' is not supported!", symbol.second->type); break;
        }
        hash_combine(state_hash, symbol_hash); // Combine with the overall state
    }
    return state_hash;
}

bool TTA::SetCurrentState(const State& newstate) {
    for(auto& componentLocation : newstate.componentLocations) {
        auto compit = components.find(componentLocation.first);
        if(compit != components.end())
            compit->second.currentLocation = componentLocation.second;
        else {
            spdlog::critical("Attempted to change the state of TTA failed. Component '{0}' does not exist.",
                             componentLocation.first);
            return false;
        }
    }
    for(auto& symbol : newstate.symbols.map()) {
        auto symbolit = symbols.map().find(symbol.first);
        bool error = false;
        if(symbolit == symbols.map().end()) { spdlog::critical("Attempted to change the state of TTA failed. Symbol '{0}' does not exist.", symbol.first); error = true; }
        else if(symbolit->second->type != symbol.second->type)  { spdlog::critical("Attempted to change the state of TTA failed. Symbol '{0}' does not have the correct type.", symbol.first); error = true; }
        if(!error) symbolit->second = symbol.second;
        else return false;
    }
    return true;
}

bool TTA::IsCurrentStateImmediate() const {
    for(auto& component : components)
        if(component.second.currentLocation.isImmediate) return true;
    return false;
}

bool TTA::IsStateImmediate(const TTA::State &state) {
    for(auto& component : state.componentLocations)
        if(component.second.isImmediate) return true;
    return false;
}

std::vector<TTA::State> TTA::GetNextTickStates(const nondeterminism_strategy_t& strategy) const {
    auto currentLocations = GetCurrentLocations();
    std::vector<UpdateExpression> symbolChanges{};

    std::multimap<std::string, UpdateExpression> symbolsToChange{};
    for(auto& component : components) {
        auto enabledEdges = component.second.GetEnabledEdges(symbols);
        if(!enabledEdges.empty()) {
            // TODO: Stop picking the first. e.g. Implement divergent behaviour. NOTE: You wanna do this with a multimap of components.
            if(enabledEdges.size() > 1) spdlog::error("Non-deterministic choice in Component '{0}'. "
                                                      "Non-deterministic strategies are not implemented yet."
                                                      "Defaulting to picking the first!", component.first);
            auto& pickedEdge = enabledEdges[0];
            bool updateInfluenceOverlap = false;
            for(auto& expr : pickedEdge.updateExpressions) {
                if(symbolsToChange.count(expr.lhs) > 0) {
                    spdlog::warn("Overlapping update influence on evaluation of update on edge {0}-->{1}. "
                                     "Variable '{2}' is already being written to in this tick!",
                                     pickedEdge.sourceLocation.identifier, pickedEdge.targetLocation.identifier,
                                     expr.lhs);
                    updateInfluenceOverlap = true;
                    break;
                }
                else
                    symbolsToChange.insert({ expr.lhs, expr });
            }
            if(updateInfluenceOverlap) continue;
            for(auto& symbolChange : symbolsToChange) symbolChanges.push_back(symbolChange.second);
            currentLocations[component.first] = pickedEdge.targetLocation;
        }
    }
    SymbolMap symbolscpy = symbols;
    for(auto& symbolChange : symbolChanges) symbolscpy[symbolChange.lhs] = symbolChange.Evaluate(symbolscpy);
    return {{ currentLocations, symbolscpy }};
}

std::string TTA::GetCurrentStateString() const {
    std::stringstream ss{};
    for(auto& component : components) ss<<component.first<<": "<<component.second.currentLocation.identifier<<"\n";
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

}
