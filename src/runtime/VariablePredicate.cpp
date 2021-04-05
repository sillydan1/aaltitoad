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
#include <runtime/VariablePredicate.h>
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

VariablePredicate::PredicateComparator VariablePredicate::ConvertFromString(const std::string &cc) {
    if(cc == "<") return PredicateComparator::LESS;
    if(cc == "<=") return PredicateComparator::LESSEQ;
    if(cc == "!=") return PredicateComparator::NEQ;
    if(cc == "==") return PredicateComparator::EQ;
    if(cc == ">") return PredicateComparator::GREATER;
    if(cc == ">=") return PredicateComparator::GREATEREQ;
    throw std::logic_error("Predicate Comparator string conversion error");
}

std::string VariablePredicate::ToGuardString() const {
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

TTASymbol_t VariablePredicate::GetValueOverTheEdge() const {
    TTASymbol_t value_over;
    std::visit(overload(
            [&](const int& v)            {
                switch (comparator) {
                    case PredicateComparator::LESS: value_over = v - 1; break;
                    case PredicateComparator::NEQ:
                    case PredicateComparator::GREATER: value_over = v + 1; break;
                    case PredicateComparator::LESSEQ:
                    case PredicateComparator::GREATEREQ:
                    case PredicateComparator::EQ: value_over = v; break;
                    default: spdlog::error("Calculating the 'over the edge' value for predicate '{0}' failed.", ToGuardString()); break;
                } },
            [&](const float& v)          {
                switch (comparator) {
                    case PredicateComparator::LESS: value_over = v - 1; break;
                    case PredicateComparator::NEQ:
                    case PredicateComparator::GREATER: value_over = v + 1; break;
                    case PredicateComparator::LESSEQ:
                    case PredicateComparator::GREATEREQ:
                    case PredicateComparator::EQ: value_over = v; break;
                    default: spdlog::error("Calculating the 'over the edge' value for predicate '{0}' failed.", ToGuardString()); break;
                } },
            [&](const bool& v)           {
                switch (comparator) {
                    case PredicateComparator::NEQ: value_over = !v; break;
                    case PredicateComparator::EQ: value_over = v; break;
                    default: spdlog::error("Calculating the 'over the edge' value for predicate '{0}' failed.", ToGuardString()); break;
                } },
            [&](const TTATimerSymbol& v) {
                value_over = TTATimerSymbol{};
                auto& vot = std::get<TTATimerSymbol>(value_over);
                switch (comparator) {
                    case PredicateComparator::LESS: vot.current_value = v.current_value - 1; break;
                    case PredicateComparator::NEQ:
                    case PredicateComparator::GREATER: vot.current_value = v.current_value + 1; break;
                    case PredicateComparator::LESSEQ:
                    case PredicateComparator::GREATEREQ:
                    case PredicateComparator::EQ: vot.current_value = v.current_value; break;
                    default: spdlog::error("Calculating the 'over the edge' value for predicate '{0}' failed.", ToGuardString()); break;
                } },
            [&](const std::string& v)    {
                switch (comparator) {
                    case PredicateComparator::NEQ: value_over = v+"Something else"; break; /// This is hilarious
                    case PredicateComparator::EQ: value_over = v; break;
                    default: spdlog::error("Calculating the 'over the edge' value for predicate '{0}' failed.", ToGuardString()); break;
                } }
    ), value);
    return value_over;
}

TTASymbol_t VariablePredicate::GetValueOnTheEdge() const {
    TTASymbol_t value_over;
    std::visit(overload(
            [&](const int& v)            {
                switch (comparator) {
                    case PredicateComparator::LESSEQ:
                    case PredicateComparator::EQ:
                    case PredicateComparator::LESS: value_over = v + 1; break;
                    case PredicateComparator::NEQ: value_over = v; break;
                    case PredicateComparator::GREATEREQ:
                    case PredicateComparator::GREATER: value_over = v - 1; break;
                    default: spdlog::error("Calculating the 'over the edge' value for predicate '{0}' failed.", ToGuardString()); break;
                } },
            [&](const float& v)          {
                switch (comparator) {
                    case PredicateComparator::LESSEQ:
                    case PredicateComparator::EQ:
                    case PredicateComparator::LESS: value_over = v + 1; break;
                    case PredicateComparator::NEQ: value_over = v; break;
                    case PredicateComparator::GREATEREQ:
                    case PredicateComparator::GREATER: value_over = v - 1; break;
                    default: spdlog::error("Calculating the 'over the edge' value for predicate '{0}' failed.", ToGuardString()); break;
                } },
            [&](const bool& v)           {
                switch (comparator) {
                    case PredicateComparator::NEQ: value_over = v; break;
                    case PredicateComparator::EQ: value_over = !v; break;
                    default: spdlog::error("Calculating the 'over the edge' value for predicate '{0}' failed.", ToGuardString()); break;
                } },
            [&](const TTATimerSymbol& v) {
                value_over = TTATimerSymbol{};
                auto& vot = std::get<TTATimerSymbol>(value_over);
                switch (comparator) {
                    case PredicateComparator::LESSEQ:
                    case PredicateComparator::EQ:
                    case PredicateComparator::LESS: vot.current_value = v.current_value + 1; break;
                    case PredicateComparator::NEQ: vot.current_value = v.current_value; break;
                    case PredicateComparator::GREATEREQ:
                    case PredicateComparator::GREATER: vot.current_value = v.current_value - 1; break;
                    default: spdlog::error("Calculating the 'over the edge' value for predicate '{0}' failed.", ToGuardString()); break;
                } },
            [&](const std::string& v)    {
                switch (comparator) {
                    case PredicateComparator::EQ: value_over = v+"Something else"; break; /// This is hilarious
                    case PredicateComparator::NEQ: value_over = v; break;
                    default: spdlog::error("Calculating the 'over the edge' value for predicate '{0}' failed.", ToGuardString()); break;
                } }
    ), value);
    return value_over;
}
