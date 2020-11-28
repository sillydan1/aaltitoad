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
#include "VariablePredicate.h"
#include <extensions/not_implemented_yet_exception.h>

std::string VariablePredicate::ConvertToString(const PredicateComparator& cc) {
    switch (cc) {
        case PredicateComparator::LESS:      return "<";
        case PredicateComparator::LESSEQ:    return "<=";
        case PredicateComparator::NEQ:       return "!=";
        case PredicateComparator::EQ:        return "==";
        case PredicateComparator::GREATER:   return ">";
        case PredicateComparator::GREATEREQ: return ">=";
        default: spdlog::critical("Predicate Comparator string conversion error");
            return "ERROR";
    }
}

std::string VariablePredicate::ToGuardString() {
    std::stringstream ss{};
    ss << variable << " " << ConvertToString(comparator) << " ";
    std::visit(overload(
            [&ss](const int& v)            { ss << v; },
            [&ss](const float& v)          { ss << v; },
            [&ss](const bool& v)           { ss << v; },
            [&ss](const TTATimerSymbol& v) { ss << v.current_value; },
            [&ss](const std::string& v)    { ss << v; },
            [&ss](const auto&& v)          { ss << "ERROR"; }
    ), value);
    return ss.str();
}

VariablePredicate VariablePredicate::Merge(const VariablePredicate& a, const VariablePredicate& b) {
    spdlog::error("VariablePredicate::Merge function is not implemented yet!");
    throw not_implemented_yet_exception();
    // TODO: Implement this
    return VariablePredicate();
}
