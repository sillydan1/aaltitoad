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
#ifndef AALTITOAD_VARIABLEPREDICATE_H
#define AALTITOAD_VARIABLEPREDICATE_H
#include "TTASymbol.h"
#include <extensions/overload>
#include <aaltitoadpch.h>

/**
 * Contains predicates such as
 * my_external_variable > 8
 * This is used for symbolic model checking of external input variables
 * */
struct VariablePredicate {
    enum class PredicateComparator {
        LESS, LESSEQ,
        NEQ, EQ,
        GREATER, GREATEREQ
    };
    std::string variable;
    PredicateComparator comparator;
    TTASymbol_t value;
    static std::string ConvertToString(const PredicateComparator& cc);
    std::string ToGuardString() const;
    static VariablePredicate Merge(const VariablePredicate& a, const VariablePredicate& b);

    TTASymbol_t GetValueOverTheEdge() const;

};

#endif //AALTITOAD_VARIABLEPREDICATE_H
