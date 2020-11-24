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

#ifndef MAVE_QUERYTYPES_H
#define MAVE_QUERYTYPES_H
#include <mavepch.h>

#include <utility>
///// GRAMMAR:
// Query ::= E quantifier | A quantifier
// quantifier ::= X phi | G phi | F phi | U phi
// --------------------------------------------------------
// phi ::= phi and phi | phi or phi | psi | deadlock
// psi ::= comparable <X> comparable | loc
// loc ::= Comp(param).Loc
// comparable ::= varname | constant
// constant ::= [0-9]+ | strings

struct Condition {
    std::string expression;
    Condition(const std::string& _expression) : expression{_expression} {}
    virtual bool ContainsLocationNames() { return false; }
};
// As in CON-Junction and DIS-Junction
enum struct Junction {
    AND = 0, OR = 0
};
struct LogicCondition : Condition {
    Junction junction;
    Condition lhs, rhs;
    LogicCondition(Condition left, const Junction& _junction, Condition right)
      : Condition{""}, lhs{std::move(left)}, junction{_junction}, rhs{std::move(right)} {}
    bool ContainsLocationNames() override { return lhs.ContainsLocationNames() || rhs.ContainsLocationNames(); }
};
struct LocationCondition : Condition {
    LocationCondition(const std::string& _expression) : Condition{_expression} {}
    bool ContainsLocationNames() override { return true; }
};
struct DeadlockCondition : Condition {
    DeadlockCondition() : Condition{"deadlock"} {}
};

// Quantifiers describe the search strategy of the verification engine.
// A's are offset by 10 in order to do some parsing wizardry
enum struct Quantifier : unsigned int {
    EX =  0, EF =  1, EG =  2, EU =  3,
    AX = 10, AF = 11, AG = 12, AU = 13
};

struct Query {
    Quantifier quantifier;
    Condition condition;
    Query(const Quantifier& _quantifier, Condition  _condition) : quantifier{_quantifier}, condition{std::move(_condition)} {}
};

#endif //MAVE_QUERYTYPES_H
