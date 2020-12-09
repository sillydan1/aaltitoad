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
#ifndef AALTITOAD_TTASUCCESSORGENERATOR_H
#define AALTITOAD_TTASUCCESSORGENERATOR_H
#include <aaltitoadpch.h>
#include <runtime/TTA.h>

class TTASuccessorGenerator {
public:
    /// Gets the set of states that are reachable in the given state (no tocking implications)
    static std::vector<TTA::StateChange> GetNextTickStates(const TTA& ttaStateAndGraph);
    /// Gets all the predicates
    static std::vector<VariablePredicate> GetInterestingVariablePredicatesInState(const TTA& ttaState);
    /// A more efficient way of checking if the state is interesting, than getting an empty vector
    static bool IsStateInteresting(const TTA& ttaState);
    /// Finds and applies all available interesting predicates
    static std::vector<TTA::StateChange> GetNextTockStates(const TTA& ttaState);

private:
    static VariablePredicate ConvertFromGuardExpression(const TTA::GuardExpression& expressionTree, const TTA& ttaState);
};

#endif //AALTITOAD_TTASUCCESSORGENERATOR_H
