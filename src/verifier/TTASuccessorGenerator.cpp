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
#include <extensions/tree_extensions.h>
#include <extensions/stringextensions.h>
#include "TTASuccessorGenerator.h"

std::vector<TTA::StateChange> TTASuccessorGenerator::GetNextTickStates(const TTA &tta) {
    return tta.GetNextTickStates(nondeterminism_strategy_t::VERIFICATION);
}

std::vector<VariablePredicate> TTASuccessorGenerator::GetInterestingVariablePredicatesInState(const TTA &ttaState) {
    // Get all edges that we may be able to take.
    auto currentEdges = ttaState.GetCurrentEdges();
    // Filter over the "interesting" edges
    currentEdges.erase(std::remove_if(currentEdges.begin(), currentEdges.end(),
                                      [](const auto& edge){ return !edge.ContainsExternalChecks(); }), currentEdges.end());
    // Extract predicates based on the guards of those edges
    std::vector<VariablePredicate> preds{};
    for(auto& edge : currentEdges) {
        for(auto& expr : edge.externalGuardCollection)
            preds.push_back(ConvertFromGuardExpression(expr, ttaState));
    }
    return preds; // TODO: Check for uniqueness and/or satisfiability
}

VariablePredicate TTASuccessorGenerator::ConvertFromGuardExpression(const TTA::GuardExpression &expressionTree, const TTA& ttaState) {
    std::optional<VariablePredicate> predicate;
    bool searchFoundComparator = false;
    auto conversionAlgorithm = [&predicate, &searchFoundComparator, &ttaState] (const Tree<ASTNode>& n) {
        if(searchFoundComparator) return;
        switch (n.root.type) {
            case NodeType_t::CompLess:
            case NodeType_t::CompLessEq:
            case NodeType_t::CompNeq:
            case NodeType_t::CompEq:
            case NodeType_t::CompGreater:
            case NodeType_t::CompGreaterEq: {
                auto calcval = calculator::calculate(ConvertASTToString(n.children[1]).c_str(), ttaState.GetSymbols());
                TTASymbol_t predRHS{};
                switch (calcval.token()->type) {
                    case STR:   predRHS = calcval.asString(); break;
                    case REAL:  predRHS = static_cast<float>(calcval.asDouble()); break;
                    case INT:   predRHS = static_cast<int>(calcval.asInt()); break;
                    case BOOL:  predRHS = calcval.asBool(); break;
                    case TIMER: predRHS = TTATimerSymbol{.current_value = static_cast<float>(calcval.asDouble()) }; break;
                    default: spdlog::critical("Right hand side of expression is weird"); break;
                }
                predicate = VariablePredicate{.variable   = n.children[0].root.token,
                                              .comparator = VariablePredicate::ConvertFromString(trim_copy(n.root.token)),
                                              .value      = predRHS};
                searchFoundComparator = true;
                break;
            }
            default: break;
        }
    };
    expressionTree.tree_apply(conversionAlgorithm);
    if(predicate.has_value())
        return predicate.value();
    else {
        spdlog::critical("Conversion of guard expression '{0}' didn't work!", ConvertASTToString(expressionTree));
        return {};
    }
}

bool TTASuccessorGenerator::IsStateInteresting(const TTA& ttaState) {
    auto currentEdges = ttaState.GetCurrentEdges();
    return std::any_of(currentEdges.begin(), currentEdges.end(),
                [](const auto& edge){ return edge.ContainsExternalChecks(); });
}

using VariableValueCollection = std::set<std::pair<std::string, TTASymbol_t>>;
using VariableValueVector = std::vector<std::pair<std::string, TTASymbol_t>>;

void AssignVariable(TTA::SymbolMap& symbols, const TTA::SymbolMap& derivedSymbols, const std::string &varname, const TTASymbol_t &newValue) {
    std::visit(overload(
            [&](const int& v)            { symbols.map()[varname] = v; },
            [&](const float& v)          {
                symbols.map()[varname] = v;
                },
            [&](const bool& v)           { symbols.map()[varname] = v; },
            // TODO: We should delay all timers, not just this single one...
            [&](const TTATimerSymbol& v) {
                auto current = static_cast<float>(derivedSymbols.map()[varname].asDouble());
                float delta = std::abs(current - v.current_value);
                spdlog::critical("Delaying all timers {0}", delta);
                for(auto& s : derivedSymbols.map()) {
                    if(s.second->type == TIMER)
                        symbols.map()[s.first] = s.second; // This is a cpy, right?
                }
                TTA::StateChange::DelayTimerSymbols(symbols, delta);
                // symbols.map()[varname] = packToken(v.current_value, PACK_IS_TIMER);
            },
            [&](const std::string& v)    { symbols.map()[varname] = v; }
    ), newValue);
}
/// This absolutely explodes into a billion pieces if the sizeof(a) or b becomes too large.
/// i.e. just 16 changes equals 65536 stateChanges (2^N)
/// - which is not something that doesnt happen
/// Also, this approach is not very memory efficient, so the size limit will be even more stringed
std::vector<TTA::StateChange> bfs(const VariableValueVector& a, const VariableValueVector& b, const TTA::SymbolMap& derivedSymbols) {
    std::vector<TTA::StateChange> crossProduct{};
    std::stack<std::pair<TTA::StateChange, int>> frontier{};
    frontier.push(std::make_pair(TTA::StateChange{},0));
    while(!frontier.empty()) {
        auto statechange = frontier.top();
        frontier.pop();
        auto& curr = statechange.first;
        auto& idx  = statechange.second;
        if(idx >= a.size()) {
            crossProduct.push_back(statechange.first);
        } else {
            TTA::StateChange stA{};
            AssignVariable(stA.symbols, derivedSymbols, a[idx].first, a[idx].second);
            TTA::StateChange stB{};
            AssignVariable(stB.symbols, derivedSymbols, a[idx].first, a[idx].second);
            frontier.push(std::make_pair(curr + stA, idx+1));
            frontier.push(std::make_pair(curr + stB, idx+1));
        }
    }
    return crossProduct;
}

/// PERSONAL_NOTE: This only changes the variables. No component locations
std::vector<TTA::StateChange> TTASuccessorGenerator::GetNextTockStates(const TTA &ttaState) {
    // Get all the interesting variable predicates
    auto interestingVarPredicates = GetInterestingVariablePredicatesInState(ttaState);
    if(interestingVarPredicates.empty()) return {};
    VariableValueCollection positives{};
    VariableValueCollection negatives{};
    for (auto &predicate : interestingVarPredicates) {
        positives.insert(std::make_pair(predicate.variable, predicate.GetValueOverTheEdge()));
        negatives.insert(std::make_pair(predicate.variable, predicate.GetValueOnTheEdge()));
    }
    spdlog::critical("Trying to be proper with {0}", positives.size());
    if(positives.size() < 16) {
        VariableValueVector ps{positives.begin(), positives.end()};
        VariableValueVector ns{negatives.begin(), negatives.end()};
        std::vector<TTA::StateChange> allPermutations = bfs(ps, ns, ttaState.GetSymbols());
        return allPermutations;
    }
    // TODO: This is technically incorrect. These state changes may have an effect on the reachable state space if they are applied together
    std::vector<TTA::StateChange> allChanges{};
    for(auto& positive : positives) {
        TTA::StateChange stP{}; // Positive path
        AssignVariable(stP.symbols, ttaState.GetSymbols(), positive.first, positive.second);
        allChanges.push_back(stP);
    }
    for(auto& negative : negatives) {
        TTA::StateChange stN{}; // Negative path
        AssignVariable(stN.symbols, ttaState.GetSymbols(), negative.first, negative.second);
        allChanges.push_back(stN);
    }
    spdlog::critical("Tock Changes: {0}", allChanges.size());
    /*
    // Get all the positive and negative values that the predicates describe
    VariableValueCollection positives{};
    VariableValueCollection negatives{};
    for(auto& predicate : interestingVarPredicates) {
        positives.emplace_back(predicate.variable, predicate.GetValueOverTheEdge());
        negatives.emplace_back(predicate.variable, predicate.GetValueOnTheEdge());
    }
    // Apply the cross product of all negatives, and positives.
    // TODO: We should do some Z3 SAT solving instead of this.
    // If positives.size() == 64, then we will result in 18,446,744,073,709,551,616 permutations. This is bad.
    // std::vector<TTA::StateChange> allPermutations = bfs(positives, negatives);
    return allPermutations;
     */
    return allChanges;
}
