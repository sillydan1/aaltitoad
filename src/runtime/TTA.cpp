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
#include "TTA.h"
#include <extensions/overload>
#include <extensions/cparse_extensions.h>
#include <tinytimer/Timer.hpp>

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
                else spdlog::error("Value '{0}' is not of boolean type", valuestr); // TODO: true/false is case-sensitive. Is this good?
            },
            [&valuestr, &value](const TTATimerSymbol&) { value = TTATimerSymbol{std::stof(valuestr)}; },
            [&valuestr, &value](const std::string&){
                if(valuestr[0] == '\"' && valuestr[valuestr.size()-1] == '\"')
                    value = valuestr.substr(1,valuestr.size()-2);
                else {
                    spdlog::error("Missing '\"' on string value '{0}' - Adding them manually, but this should be corrected", valuestr);
                    value = valuestr;
                }
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
    // TODO: Maybe this should be cached
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
        auto x = symbol.second->type;
        auto y = symbolit->second->type;
        if(symbolit == symbols.map().end()) { spdlog::critical("Attempted to change the state of TTA failed. Symbol '{0}' does not exist.", symbol.first); error = true; }
        else if(!(NUM & x & y) && !(x == VAR && (NUM & y))) {
            auto a = tokenTypeToString(symbolit->second->type);
            auto b = tokenTypeToString(symbol.second->type);
            spdlog::critical(
                    "Attempted to change the state of TTA failed. Symbol '{0}' does not have the correct type. ({1} vs {2} (a := b))",
                    symbol.first, a, b);
            error = true;
        }
        if(!error) symbols.map()[symbol.first] = symbol.second;
        else return false;
    }
    tickCount++;
    return true;
}

bool TTA::IsCurrentStateImmediate() const {
    return std::any_of(components.begin(), components.end(), [](const auto& c){ return c.isImmedate; });
}

bool TTA::IsStateImmediate(const TTA::State &state) {
    return std::any_of(state.componentLocations.begin(), state.componentLocations.end(), [](const auto& c){ return c.isImmediate; });
}

TTA::InterestingStateCollection TTA::GetNextTickWithInterestingness(const nondeterminism_strategy_t& strategy) const {
    auto currentLocations = GetCurrentLocations();
    std::vector<UpdateExpression> symbolChanges{};

    std::multimap<std::string, UpdateExpression> symbolsToChange{};
    std::map<std::string, std::vector<std::string>> overlappingComponents{}; // expr.lhs -> componentIdentifiers mapping
    bool updateInfluenceOverlapGlobal = false;
    for(auto& component : components) {
        auto enabledEdges = component.second.GetEnabledEdges(symbols);
        if(!enabledEdges.empty()) {
            
            // TODO: Stop picking the first. e.g. Implement divergent behaviour. NOTE: You wanna do this with a multimap of components.
            if(enabledEdges.size() > 1) {
                spdlog::error("Non-deterministic choice in Component '{0}'.",
                              TTAResugarizer::Resugar(component.first));
                spdlog::debug("Enabled edges in component '{0}':", TTAResugarizer::Resugar(component.first));
                for(auto& e : enabledEdges)
                    spdlog::debug("{0} --> {1}",
                                  TTAResugarizer::Resugar(e.sourceLocation.identifier),
                                  TTAResugarizer::Resugar(e.targetLocation.identifier));
                spdlog::debug("----- / -----");
            }
            auto& pickedEdge = PickEdge(enabledEdges, strategy);
            bool updateInfluenceOverlap = false;
            for(auto& expr : pickedEdge.updateExpressions) {
                overlappingComponents[expr.lhs].push_back(TTAResugarizer::Resugar(
                        expr.lhs + " : " +
                        pickedEdge.sourceLocation.identifier + " --> " +
                        pickedEdge.targetLocation.identifier));
                if(symbolsToChange.count(expr.lhs) > 0) {
                    spdlog::warn("Overlapping update influence on evaluation of update on edge {0} --> {1}. "
                                 "Variable '{2}' is already being written to in this tick! - For more info, run with higher verbosity",
                                 TTAResugarizer::Resugar(pickedEdge.sourceLocation.identifier),
                                 TTAResugarizer::Resugar(pickedEdge.targetLocation.identifier),
                                 TTAResugarizer::Resugar(expr.lhs)); // TODO: Idempotent variable assignment
                    updateInfluenceOverlap = true;
                    updateInfluenceOverlapGlobal = true;
                    continue;
                }
                else
                    symbolsToChange.insert({ expr.lhs, expr });
            }
            if(!CLIConfig::getInstance()["ignore-update-influence"] && updateInfluenceOverlap) continue;
            for(auto& symbolChange : symbolsToChange) symbolChanges.push_back(symbolChange.second);
            // If we transition to the end location, immediately change to first location
            if(component.second.endLocation.identifier == pickedEdge.targetLocation.identifier)
                currentLocations[component.first] = component.second.initialLocation;
            else
                currentLocations[component.first] = pickedEdge.targetLocation;
        }
    }
    if(updateInfluenceOverlapGlobal) {
        spdlog::debug("Overlapping Components: (Tick#: {0})", tickCount);
        for (auto& componentCollection : overlappingComponents) {
            if (componentCollection.second.size() > 1) {
                for (auto& compname : componentCollection.second) {
                    spdlog::debug("{0}", compname);
                }
            }
        }
    }
    SymbolMap symbolsCopy{};
    for(auto& symbolChange : symbolChanges) {
        if(symbols.map()[symbolChange.lhs]->type == TIMER)
            symbolsCopy[symbolChange.lhs] = packToken(symbolChange.Evaluate(symbols).asDouble(), PACK_IS_TIMER);
        else
            symbolsCopy[symbolChange.lhs] = symbolChange.Evaluate(symbols);
    }
    return {{ currentLocations, symbolsCopy }};
}

// TODO: Clean this function up, it stinks!
// TODO: We need something in the query checking that can check for deadlock-ness (other than just calling this function)
std::vector<TTA::State> TTA::GetNextTickStates(const nondeterminism_strategy_t& strategy) const {
    auto interestingState = GetNextTickWithInterestingness(strategy);
    std::vector<TTA::State> thing{};
    for(auto& state : interestingState)
        thing.emplace_back(std::move(state.first));
    return thing;
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
    Timer<int> timer;
    timer.start();
    SetCurrentState(GetNextTickStates(nondeterminismStrategy)[0]);
    spdlog::info("Tick {0} time elapsed: {1} ms - (With printing and everything)", tickCount, timer.milliseconds_elapsed());
    tickCount++;
}

void TTA::InsertExternalSymbols(const TTA::SymbolMap& externalSymbolKeys) {
    symbols.map().insert(externalSymbolKeys.map().begin(), externalSymbolKeys.map().end());
    for(auto& elem : externalSymbolKeys.map())
        externalSymbols[elem.first] = symbols.find(elem.first);
}

void TTA::InsertInternalSymbols(const TTA::SymbolMap &internalSymbols) {
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

void TTA::SetAllTimers(double exactTime) {
    for(auto& symbol : symbols.map()) {
        if(symbol.second->type == TIMER)
            symbols[symbol.first] = packToken(exactTime, PACK_IS_TIMER);
    }
}
