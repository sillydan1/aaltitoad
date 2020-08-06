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
#ifndef MAVE_TTATYPES_H
#define MAVE_TTATYPES_H
#include <mavepch.h>

struct TTAIR_t {
    struct Edge {
        std::string sourceLocationName;
        std::string targetLocationName;
        std::string guardExpression;
        std::string updateExpression;
        std::string parentComponentNamePrefix;
    };
    struct SubComponent {
        std::string name;
        std::string parentComponentName;
    };
    struct Component {
        std::vector<Edge> edges;
        std::vector<SubComponent> subComponents;

    };

    // List of folded components
    // List of folded symbols
};

struct TTA_t {
    // List of reduced and unfolded components
    // list of unfolded symbols
};

#endif //MAVE_TTATYPES_H
