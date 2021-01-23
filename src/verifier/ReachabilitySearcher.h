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

#ifndef MAVE_REACHABILITYSEARCHER_H
#define MAVE_REACHABILITYSEARCHER_H
#include <verifier/query_parsing/CTLQueryParser.h>
#include <runtime/TTA.h>

struct QueryResultPair {
    bool answer;
    const Query* query;
    size_t acceptingStateHash;
};

struct SearchState {
    TTA tta;
    size_t prevStateHash;
    bool justTocked;
};

class ReachabilitySearcher {
    using StateList = std::unordered_map<size_t, SearchState>;
    StateList Passed;
    StateList Waiting;
    std::vector<QueryResultPair> query_results;
public:
    ReachabilitySearcher(const std::vector<const Query*>& queries, const TTA& initialState);
    inline bool Search() { return ForwardReachabilitySearch(); }
    void AddToWaitingList(const TTA& state, const std::vector<TTA::StateChange>& statechanges, bool justTocked);
private:
    static bool IsSearchStateTockable(const SearchState& state);
    bool AreQueriesAnswered(const std::vector<QueryResultPair>& qres);
    bool IsQuerySatisfied(const Query& query, const TTA& state);
    void AreQueriesSatisfied(std::vector<QueryResultPair>& queries, const TTA& state);
    bool ForwardReachabilitySearch();
    void PrintResults(const std::vector<QueryResultPair>& results);
};

#endif //MAVE_REACHABILITYSEARCHER_H
