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
#ifndef MAVE_SYMBOLCHANGE_H
#define MAVE_SYMBOLCHANGE_H
#include <aaltitoadpch.h>
#include "runtime/tta/tta.h"

struct SymbolChange {
    std::string identifier;
    TTASymbol_t value;
#ifndef NDEBUG
    std::string valueExpression;
#endif
};

#endif //MAVE_SYMBOLCHANGE_H
