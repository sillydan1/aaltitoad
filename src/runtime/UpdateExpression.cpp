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
#include "UpdateExpression.h"
#include <extensions/stringextensions.h>

UpdateExpression::UpdateExpression(const std::string &fullExpr) {
    auto tokens = split(fullExpr, ":=");
    if(tokens.size() != 2) {
        spdlog::critical("Update expression '{0}' is required to only have a "
                         "right-hand side and a left-hand side, delimited by the ':=' token",
                         fullExpr);
    }
    lhs = trim_copy(tokens[0]);
    rhs = trim_copy(tokens[1]);
}

std::vector<UpdateExpression> UpdateExpression::ParseExpressions(const std::string& fullExpr) {
    std::vector<UpdateExpression> expressions{};
    auto it = fullExpr.find_first_of(":=");
    if(it != std::string::npos) {
        auto subExpressions = split(fullExpr, ';');
        for(auto& expr : subExpressions)
            subExpressions.emplace_back(expr);
    } else {
        spdlog::critical("Update expression '{0}' does not include the ':=' token!", fullExpr);
    }
    return expressions;
}
