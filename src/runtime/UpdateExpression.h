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
#ifndef MAVE_UPDATEEXPRESSION_H
#define MAVE_UPDATEEXPRESSION_H
#include <aaltitoadpch.h>
#include <shunting-yard.h>

struct UpdateExpression {
    std::string lhs, rhs; // TODO: Store rhs as a compiled tree.
    explicit UpdateExpression(const std::string& fullExpr);
    static std::vector<UpdateExpression> ParseExpressions(const std::string& fullExpr);
    [[nodiscard]] packToken Evaluate(const TokenMap& map) const;
};

#endif //MAVE_UPDATEEXPRESSION_H
