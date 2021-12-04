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
#include <verifier/ReachabilitySearcher.h>
#include <verifier/TTASuccessorGenerator.h>

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
    if(query.root.type == NodeType_t::Forall && query.children.begin()->root.type == NodeType_t::Globally) {
        auto invertedQ = Query(ASTNode{NodeType_t::Negation, "!"});
        invertedQ.insert(query);
        return IsQuerySatisfiedHelper(invertedQ, state);
    }
    if(query.root.type != NodeType_t::Exists) {
        spdlog::critical("Only reachability queries are supported right now, sorry.");
        return false;
    }
    return IsQuerySatisfiedHelper(query, state);
}

void ReachabilitySearcher::AreQueriesSatisfied(std::vector<QueryResultPair>& queries, const TTA& state, size_t state_hash) {
    for(auto & query : queries) {
        if(!query.answer) {
            query.answer = IsQuerySatisfied(*query.query, state);
            if (query.answer) {
                query.acceptingStateHash = state_hash;
                auto ss = ConvertASTToString(*query.query);
                spdlog::info("Query '{0}' is satisfied!", ss);
                spdlog::debug("Query '{0}' was satisfied in state: \n{1}", ss, state.GetCurrentStateString());
            }
        }
    }
}

void ReachabilitySearcher::OutputResults(const std::vector<QueryResultPair>& results) {
    if(CLIConfig::getInstance()["output"]) {
        std::ofstream outputFile{CLIConfig::getInstance()["output"].as_string(), std::ofstream::trunc};
        for(auto& r : results) {
            outputFile << ConvertASTToString(*r.query) << " : " << std::boolalpha << r.answer << "\n";
        }
    }
}

void ReachabilitySearcher::PrintResults(const std::vector<QueryResultPair>& results) {
    OutputResults(results);
    spdlog::info("==== QUERY RESULTS ====");
    for(const auto& r : results) {
        spdlog::info("===================="); // Delimiter to make it easier to read
        spdlog::info("{0} : {1}", ConvertASTToString(*r.query), r.answer);
        auto stateHash = r.acceptingStateHash;
        std::vector<std::string> trace{};
        while(stateHash != 0) { // 0 indicates "no parent"
            spdlog::trace("{0}", stateHash);
            auto prevState = Passed.find(stateHash);
            if(prevState != Passed.end()) {
                trace.push_back(prevState->second.tta.GetCurrentStateString());
                stateHash = Passed[stateHash].prevStateHash;
            } else {
                spdlog::critical("Unable to resolve witnessing trace");
                break;
            }
        }
        std::reverse(trace.begin(), trace.end());
        for(auto& state : trace)
            spdlog::info("{0}", state);
    }
}

bool ReachabilitySearcher::ForwardReachabilitySearch(const nondeterminism_strategy_t& strategy) {
    auto stateit = Waiting.begin();
    while(stateit != Waiting.end()) {
        auto& state = stateit->second;
        auto curstatehash = stateit->first;
        AreQueriesSatisfied(query_results, state.tta, curstatehash);
        if(AreQueriesAnswered(query_results)) {
            Passed[curstatehash] = Waiting[curstatehash];
            PrintResults(query_results);
            spdlog::info("Found a positive result after searching: {0} states", Passed.size());
            return true; // All the queries has been reached
        }
        // If the state is interesting, apply tock changes
        // TODO: Guards with parentheses that checks on interesting variables are not parsed properly. Expect weird behavior
        if(IsSearchStateTockable(state) && TTASuccessorGenerator::IsStateInteresting(state.tta)) {
            auto allTockStateChanges = TTASuccessorGenerator::GetNextTockStates(state.tta);
            AddToWaitingList(state.tta, allTockStateChanges, true, curstatehash);
        }
        auto allTickStateChanges = TTASuccessorGenerator::GetNextTickStates(state.tta);
        AddToWaitingList(state.tta, allTickStateChanges, false, curstatehash);
        Passed[curstatehash] = Waiting[curstatehash];
        Waiting.erase(curstatehash);
        stateit = PickStateFromWaitingList(strategy);
    }
    PrintResults(query_results);
    spdlog::info("Found a negative result after searching: {0} states", Passed.size());
    return false;
}

ReachabilitySearcher::ReachabilitySearcher(const std::vector<const Query *> &queries, const TTA &initialState)
 : Passed{}, Waiting{}, query_results{}
{
    query_results.reserve(queries.size());
    for(auto& q : queries) query_results.push_back({.answer = false, .query = q, .acceptingStateHash = 0});
    Waiting[initialState.GetCurrentStateHash()] = SearchState{initialState, 0, false};
}

void ReachabilitySearcher::AddToWaitingList(const TTA &state, const std::vector<TTA::StateChange> &statechanges, bool justTocked, size_t state_hash) {
    for(auto& change : statechanges) {
        /// This is a lot of copying large data objects... Figure something out with maybe std::move
        auto nstate = state << change;
        auto nstatehash = nstate.GetCurrentStateHash();
        if(Passed.find(nstatehash) == Passed.end())
            Waiting[nstatehash] = SearchState{nstate, state_hash, justTocked};
    }
}

bool ReachabilitySearcher::AreQueriesAnswered(const std::vector<QueryResultPair> &qres) {
    return std::all_of(query_results.begin(), query_results.end(), [](const auto& r){ return r.answer; });
}

bool ReachabilitySearcher::IsSearchStateTockable(const SearchState& state) {
    return (!state.justTocked && !state.tta.IsCurrentStateImmediate());
}

ReachabilitySearcher::StateList::iterator ReachabilitySearcher::PickStateFromWaitingList(const nondeterminism_strategy_t& strategy) {
    if(Waiting.empty()) return Waiting.end();
    if(Waiting.size() == 1) return Waiting.begin();
    switch (strategy) {
        case nondeterminism_strategy_t::PANIC:
            throw std::logic_error("Panicking on nondeterminism");
        case nondeterminism_strategy_t::VERIFICATION:
        case nondeterminism_strategy_t::PICK_FIRST:
        case nondeterminism_strategy_t::PICK_LAST:
            return Waiting.begin(); // There's not a concept of "last" or "first" in a hashmap
        case nondeterminism_strategy_t::PICK_RANDOM:
            auto randomPick = rand() % Waiting.size()-1;
            auto picked = Waiting.begin();
            for(int i = 0; i < randomPick; i++) picked++;
            return picked;
    }
    return Waiting.end();
}
