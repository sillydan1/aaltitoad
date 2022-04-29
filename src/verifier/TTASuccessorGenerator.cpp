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
#include <verifier/TTASuccessorGenerator.h>
#include <cmath>

std::vector<TTA::StateChange> TTASuccessorGenerator::GetNextTickStates(const TTA &tta) {
    return tta.GetNextTickStates(nondeterminism_strategy_t::VERIFICATION);
}

std::vector<std::vector<VariablePredicate>> TTASuccessorGenerator::GetInterestingVariablePredicatesInState(const TTA &ttaState) {
    // Get all edges that we may be able to take.
    auto currentEdges = ttaState.GetCurrentEdges();
    // Filter over the "interesting" edges
    currentEdges.erase(std::remove_if(currentEdges.begin(), currentEdges.end(),
                                      [](const auto& edge){ return !edge.ContainsExternalChecks(); }), currentEdges.end());
    // Extract predicates based on the guards of those edges
    std::vector<std::vector<VariablePredicate>> all_preds{};
    for(auto& edge : currentEdges) {
        std::vector<VariablePredicate> preds{};
        for(auto& expr : edge.externalGuardCollection)
            preds.push_back(ConvertFromGuardExpression(expr, ttaState));
        all_preds.push_back(preds);
    }
    return all_preds; // TODO: Check for uniqueness and/or satisfiability
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
                    case STR: predRHS = calcval.asString(); break;
                    case REAL: predRHS = static_cast<float>(calcval.asDouble()); break;
                    case INT: predRHS = static_cast<int>(calcval.asInt()); break;
                    case BOOL: predRHS = calcval.asBool(); break;
                    case TIMER: predRHS = TTATimerSymbol{.current_value = static_cast<float>(calcval.asDouble()) }; break;
                    default: spdlog::critical("Right hand side of expression is weird"); break;
                }
                predicate = VariablePredicate{.variable   = n.children[0].root.token,
                                              .comparator = VariablePredicate::ConvertFromString(trim_copy(n.root.token)),
                                              .value      = predRHS};
                searchFoundComparator = true;
                break;
            }
            case NodeType_t::Var: {
                auto calcval = ttaState.GetSymbols().find(n.root.token);
                TTASymbol_t predRHS{};
                switch (calcval->token()->type) {
                    case STR:   predRHS = calcval->asString(); break;
                    case REAL:  predRHS = static_cast<float>(calcval->asDouble()); break;
                    case INT:   predRHS = static_cast<int>(calcval->asInt()); break;
                    case BOOL:  predRHS = calcval->asBool(); break;
                    case TIMER: predRHS = TTATimerSymbol{.current_value = static_cast<float>(calcval->asDouble()) }; break;
                    default: spdlog::critical("Right hand side of expression is weird"); break;
                }
                predicate = VariablePredicate{.variable   = n.root.token,
                        .comparator = VariablePredicate::ConvertFromString("!="),
                        .value      = predRHS};
                searchFoundComparator = true;
            }
            default: break;
        }
    };
    expressionTree.tree_apply(conversionAlgorithm);
    if(predicate.has_value())
        return predicate.value();
    spdlog::critical("Conversion of guard expression '{0}' didn't work!", ConvertASTToString(expressionTree));
    return {};
}

bool TTASuccessorGenerator::IsStateInteresting(const TTA& ttaState) {
    auto currentEdges = ttaState.GetCurrentEdges();
    return std::any_of(currentEdges.begin(), currentEdges.end(),
                [](const auto& edge){ return edge.ContainsExternalChecks(); });
}

struct SymbolNamePair {
    SymbolNamePair(const std::string& varname, const TTASymbol_t& symbol)
     : varname(varname), symbol(symbol) {}
    std::string varname;
    TTASymbol_t symbol;
};
using VariableValueCollection = std::set<std::pair<std::string, TTASymbol_t>>;
using VariableValueVector = std::vector<SymbolNamePair>;

void AssignVariable(TTA::SymbolMap& outputMap, const TTA::SymbolMap& currentValues, const std::string &varname, const TTASymbol_t &newValue) {
    std::visit(overload(
            [&](const int& v)            { outputMap.map()[varname] = v; },
            [&](const long& v)           { outputMap.map()[varname] = static_cast<int64_t>(v); },
            [&](const float& v)          { outputMap.map()[varname] = v; },
            [&](const bool& v)           { outputMap.map()[varname] = v; },
            [&](const TTATimerSymbol& v) {
                auto current = static_cast<float>(currentValues.map()[varname].asDouble());
                float delta = std::abs(current - v.current_value);
                spdlog::trace("Delaying all timers {0}", delta);
                for(auto& s : currentValues.map()) {
                    if(s.second->type == TIMER)
                        outputMap.map()[s.first] = s.second;
                }
                TTA::StateChange::DelayTimerSymbols(outputMap, delta);
                // outputMap.map()[varname] = packToken(v.current_value, PACK_IS_TIMER);
            },
            [&](const std::string& v)    { outputMap.map()[varname] = v; }
    ), newValue);
}

void add(std::vector<bool>& v) {
    for(auto i = 0; i < v.size(); i++) {
        if(v[i])
            continue;
        for(; i >= 0; i--)
            v[i].flip();
        break;
    }
}

std::vector<TTA::StateChange> SymbolsCrossProduct(const VariableValueVector& positives,
                                                  const VariableValueVector& negatives,
                                                  unsigned int predicate_count,
                                                  const TTA::SymbolMap& derivedSymbols) {
    std::vector<TTA::StateChange> return_value{};
    std::vector<bool> bitset(predicate_count);
    auto size = pow(2, predicate_count);
    for(auto i = 0; i < size; i++) {
        TTA::StateChange change{};
        auto j = 0;
        for(auto b : bitset) {
            if(b)
                AssignVariable(change.symbols, derivedSymbols, positives[j].varname, positives[j].symbol);
            else
                AssignVariable(change.symbols, derivedSymbols, negatives[j].varname, negatives[j].symbol);
            j++;
        }
        return_value.push_back(change);
        add(bitset);
    }
    return return_value;
}

/// This absolutely explodes into a billion pieces if the sizeof(a) or b becomes too large.
/// i.e. just 16 changes equals 65536 stateChanges (2^N)
/// - which is not something that doesnt happen
/// Also, this approach is not very memory efficient, so the size limit will be even more stringed
std::vector<TTA::StateChange> BFSCrossProduct(const VariableValueVector& a, const VariableValueVector& b, const TTA::SymbolMap& derivedSymbols) {
    std::vector<TTA::StateChange> crossProduct{};
    std::stack<std::pair<TTA::StateChange, int>> frontier{};
    frontier.push(std::make_pair(TTA::StateChange{},0));
    while(!frontier.empty()) {
        auto statechange = frontier.top();
        frontier.pop();
        auto& curr = statechange.first;
        auto& idx  = statechange.second;
        if(idx >= a.size() || idx >= b.size()) {
            crossProduct.push_back(statechange.first);
        } else {
            TTA::StateChange stA{};
            AssignVariable(stA.symbols, derivedSymbols, a[idx].varname, a[idx].symbol);
            TTA::StateChange stB{};
            AssignVariable(stB.symbols, derivedSymbols, b[idx].varname, b[idx].symbol);
            frontier.push(std::make_pair(curr + stA, idx+1));
            frontier.push(std::make_pair(curr + stB, idx+1));
        }
    }
    return crossProduct;
}

std::vector<TTA::StateChange> TTASuccessorGenerator::GetNextTockStates(const TTA& ttaState) {
    // Get all the interesting variable predicates
    auto interestingVarPredicates = GetInterestingVariablePredicatesInState(ttaState);
    std::vector<TTA::StateChange> return_value{};
    for(auto& preds : interestingVarPredicates) {
        auto changes = GetNextTockStatesFromPredicates(preds, ttaState.GetSymbols());
        return_value.insert(return_value.end(), changes.begin(), changes.end());
    }
    return return_value;
}

std::vector<TTA::StateChange> TTASuccessorGenerator::GetNextTockStatesFromPredicates(const std::vector<VariablePredicate>& predicates, const TTA::SymbolMap& symbols) {
    if(predicates.empty())
        return {};
    VariableValueVector positives{};
    VariableValueVector negatives{};
    for (auto& predicate : predicates) {
        positives.emplace_back(predicate.variable, predicate.GetValueOverTheEdge());
        negatives.emplace_back(predicate.variable, predicate.GetValueOnTheEdge());
    }
    auto size = predicates.size();
    spdlog::trace("Size of the set of interesting changes is {0}, this means you will get {1} new states",
                  size, static_cast<int>(pow(2, size)));
    return SymbolsCrossProduct(positives, negatives, size, symbols);
}
