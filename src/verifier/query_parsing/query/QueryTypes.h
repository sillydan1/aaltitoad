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
};

struct LocationCondition : Condition {
// TODO: Something more...
};

struct DeadlockCondition : Condition {
// TODO: Something more...
};

// Quantifiers describe the search strategy of the verification engine.
struct Quantifier {
    Quantifier();
    // You can't just have a "nothing" quantifier.
    virtual ~Quantifier() = 0;
};
// TODO: Something more...
struct EXQuantifier : Quantifier {
    virtual ~EXQuantifier() override;
};

struct EFQuantifier : Quantifier {
    virtual ~EFQuantifier() override;
};

struct EGQuantifier : Quantifier {
    virtual ~EGQuantifier() override;
};

struct EUQuantifier : Quantifier {
    virtual ~EUQuantifier() override;
};

struct AXQuantifier : Quantifier {
    virtual ~AXQuantifier() override;
};

struct AGQuantifier : Quantifier {
    virtual ~AGQuantifier() override;
};

struct AFQuantifier : Quantifier {
    virtual ~AFQuantifier() override;
};

struct AUQuantifier : Quantifier {
    virtual ~AUQuantifier() override;
};

struct BaseQuery {
    Condition condition;
};

template<typename Quant>
struct Query : BaseQuery {
    static_assert(std::is_base_of_v<Quantifier, Quant>, "Provided template is not a Quantifier");
    Quant quantifier;
};

#endif //MAVE_QUERYTYPES_H
