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
#include "UpdateExpression.h"
#include <extensions/stringextensions.h>

const char* assignmentOperator = ":=";
const char  expressionDelimiter = ';';

UpdateExpression::UpdateExpression(const std::string &fullExpr) {
    auto tokens = split(fullExpr, assignmentOperator);
    if(tokens.size() != 2) {
        spdlog::critical("Update expression '{0}' is required to only have a "
                         "right-hand side and a left-hand side, delimited by the ':=' token",
                         fullExpr);
    }
    lhs = trim_copy(tokens[0]);
    rhs = trim_copy(tokens[1]);
}

std::vector<UpdateExpression> UpdateExpression::ParseExpressions(const std::string& fullExpr) {
    if(fullExpr.empty()) return {};
    std::vector<UpdateExpression> expressions{};
    if(fullExpr.find(assignmentOperator) != std::string::npos) {
        auto subExpressions = split(fullExpr, expressionDelimiter);
        for(auto& expr : subExpressions)
            expressions.push_back(UpdateExpression(expr));
    } else
        spdlog::critical("Update expression '{0}' does not include the ':=' token!", fullExpr);
    return expressions;
}

packToken UpdateExpression::Evaluate(const TokenMap& map) const {
    auto val = calculator::calculate(rhs.c_str(), map);
    auto xx = map.find(lhs)->token()->type;
    // TODO: Figure this out... if(val->type != xx) spdlog::warn("This is annoying, but I am out of time");
    switch (xx) {
        case tokType_INT: return val.asInt();
        case tokType_REAL: return val.asDouble();
        case tokType_STR: return val.asString();
        case tokType_BOOL: return val.asBool();
        case tokType_VAR: {
            return val.asBool(); // TODO: This is not right, right?
        }
        case tokType_TIMER:
            return packToken(val.asDouble(), PACK_IS_TIMER);
        case tokType_NUM: // We do not support just "numbers" in general. You need to have an actual type.
        case tokType_FUNC:
        case tokType_IT:
        case tokType_LIST:
        case tokType_TUPLE:
        case tokType_STUPLE:
        case tokType_MAP:
        case tokType_REF:
        case tokType_ANY_TYPE:
        case tokType_NONE:
        case tokType_OP:
        case tokType_UNARY:
        default: throw std::logic_error("Evaluating something we cant evaluate");
    }
}
