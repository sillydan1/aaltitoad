/**
 * Copyright (C) 2020 Asger Gitz-Johansen

   This file is part of aaltitoad.

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
#include "ReachabilitySearcher.h"
#include "TTASuccessorGenerator.h"

#include <tinytimer/Timer.hpp>
#include <extensions/tree_extensions.h>
#include <verifier/trace_output/TTAResugarizer.h>

bool IsQuerySatisfiedHelper(const Query& query, const TTA& state) {
    switch (query.root.type) {
        case NodeType_t::Location: {
            auto ddd = state.GetCurrentLocationsLocationsOnly();
            bool ret = std::find(ddd.begin(), ddd.end(), TTAResugarizer::Unsugar(query.root.token)) != ddd.end();
            return ret;
        }
        case NodeType_t::Deadlock: return state.IsDeadlocked(); //// Deadlocked and is immediate. If we are not immediate, we can still tock (unless the interesting variables set is empty)
        case NodeType_t::LogicAnd: return IsQuerySatisfiedHelper(query.children[0], state) && IsQuerySatisfiedHelper(query.children[1], state);
        case NodeType_t::LogicOr:  return IsQuerySatisfiedHelper(query.children[0], state) || IsQuerySatisfiedHelper(query.children[1], state);
        case NodeType_t::CompLess:
        case NodeType_t::CompLessEq:
        case NodeType_t::CompNeq:
        case NodeType_t::CompEq:
        case NodeType_t::CompGreater:
        case NodeType_t::CompGreaterEq: {
            std::string exprstring{}; // This string can technically be precompiled.
            query.children[0].tree_apply([&exprstring]( const ASTNode& node ){ exprstring += node.token; });
            exprstring += query.root.token;
            query.children[1].tree_apply([&exprstring]( const ASTNode& node ){ exprstring += node.token; });
            spdlog::debug("Assembled expression '{0}'", exprstring);
            calculator c(exprstring.c_str());
            return c.eval(state.GetSymbols()).asBool();
        }
        case NodeType_t::SubExpr:
        case NodeType_t::Finally:
        case NodeType_t::Globally:
        case NodeType_t::Next:
        case NodeType_t::Until:
        case NodeType_t::Exists:
        case NodeType_t::Forall:    return IsQuerySatisfiedHelper(query.children[0], state);
        case NodeType_t::Negation:  return !IsQuerySatisfiedHelper(query.children[0], state);
        // These are handled elsewhere, so they should output an error
        case NodeType_t::Literal:
        case NodeType_t::Var: // yes, even boolean valued variables are required to be on the lhs of an "== true" expression.
        case NodeType_t::UNKNOWN:
        default: spdlog::error("Something went wrong evaluating the query."); break;
    }
    return false;
}

bool ReachabilitySearcher::IsQuerySatisfied(const Query& query, const TTA &state) {
    if(query.root.type != NodeType_t::Exists) {
        spdlog::critical("Only reachability queries are supported right now, sorry.");
        return false;
    }
    return IsQuerySatisfiedHelper(query, state);
}

void ReachabilitySearcher::AreQueriesSatisfied(std::vector<QueryResultPair>& queries, const TTA& state, int dbgsize) {
    for(auto & query : queries) {
        if(!query.answer) {
            query.answer = IsQuerySatisfied(*query.query, state);
            if (query.answer)
                spdlog::info("Query '{0}' is satisfied! - {1}", ConvertASTToString(*query.query), dbgsize);
        }
    }
}

struct TTAIsTockStatePair {
    TTA tta;
    bool justTocked;
};

// TODO: Extract TTA into TTAState and TTAGraph, to minimize the memory footprint
bool ReachabilitySearcher::ForwardReachabilitySearch(const Query &query, const TTA &initialState) {
    return ForwardReachabilitySearch({&query}, initialState);
}

void PrintResults(const std::vector<QueryResultPair>& results) {
    spdlog::info("==== QUERY RESULTS ====");
    for(auto& r : results)
        spdlog::info("{0} : {1}", ConvertASTToString(*r.query), r.answer);
}

bool ReachabilitySearcher::ForwardReachabilitySearch(const std::vector<const Query*>& queries, const TTA& initialState) {
    std::vector<QueryResultPair> query_results{}; query_results.reserve(queries.size());
    for(auto& q : queries) query_results.push_back({.answer = false, .query = q});
    std::unordered_map<size_t, bool> Passed{}; // TODO: This should be stored in a manner that makes it possible to provide traces
    std::unordered_map<size_t, TTAIsTockStatePair> Waiting{};
    Waiting[initialState.GetCurrentStateHash()] = TTAIsTockStatePair{initialState, false};
    auto add_to_waiting_list = [&](const TTA& state, const std::vector<TTA::StateChange>& statechanges, bool justTocked){
        for(auto& change : statechanges) {
            /// This is a lot of copying large data objects... Figure something out with maybe std::move
            auto nstate = state << change;
            auto nstatehash = nstate.GetCurrentStateHash();
            if(Passed.find(nstatehash) == Passed.end())
                Waiting[nstatehash] = TTAIsTockStatePair{nstate, justTocked};
        }
    };
    auto stateit = Waiting.begin();
    while(stateit != Waiting.end()) {
        auto& state = stateit->second;
        auto curstatehash = stateit->first;
        AreQueriesSatisfied(query_results, state.tta, Passed.size());
        if(std::all_of(query_results.begin(), query_results.end(), [](const auto& r){ return r.answer; })) {
            PrintResults(query_results);
            spdlog::info("Found a positive result after searching: {0} states", Passed.size());
            return true; // All the queries has been reached
        }
        // If the state is interesting, apply tock changes
        // TODO: Timers being wrongly applied.
        // TODO: Guards with parentheses that checks on interesting variables are not parsed properly. Expect weird behavior
        if(!state.justTocked && !state.tta.IsCurrentStateImmediate() && TTASuccessorGenerator::IsStateInteresting(state.tta)) {
            auto allTockStateChanges = TTASuccessorGenerator::GetNextTockStates(state.tta);
            add_to_waiting_list(state.tta, allTockStateChanges, true);
        }
        auto allTickStateChanges = TTASuccessorGenerator::GetNextTickStates(state.tta);
        add_to_waiting_list(state.tta, allTickStateChanges, false);
        Passed[curstatehash] = true;
        Waiting.erase(curstatehash); // Remove this state from the waiting list
        stateit = Waiting.begin();
    }
    PrintResults(query_results);
    spdlog::info("Found a negative result after searching: {0} states", Passed.size());
    return false;
    /***  Forward reachability search algorithm
     * Passed = Ø
     * Waiting = {initial}
     *
     * pick (state) in waiting <DFS,BFS etc>
     *      if (state |= Q) return true
     *      for all (state -> state'):
     *           add (state') to waiting
     *      move (state) to Passed
     *      Until Waiting = Ø
     * return false
     * */
}


