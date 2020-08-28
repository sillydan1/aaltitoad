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

using TTASymbolType = std::variant<
        int,
        float,
        bool,
        std::string
        >;

TTASymbolType TTASymbolValueFromTypeAndValueStrings(const std::string& typestr, const std::string& valuestr);
TTASymbolType TTASymbolTypeFromString(const std::string& typestr);
TTASymbolType PopulateValueFromString(const TTASymbolType& type, const std::string& valuestr);

struct TTAIR_t {
public:
    struct Edge {
        std::string sourceLocationName;
        std::string targetLocationName;
        std::string guardExpression;
        std::string updateExpression;
    };
    struct Symbol {
        std::string identifier;
        TTASymbolType value;
    };
    struct Component {
        std::string initialLocation;
        std::string endLocation;
        bool isMain = false;
        std::vector<Edge> edges = {};
        std::vector<Symbol> symbols = {};
    };
public:
    std::vector<Component> components = {};

public:
    [[nodiscard]] std::optional<std::vector<Component>::const_iterator> FindMainComponent() const;
    void AddComponent(Component&& component);

private:
    bool hasMainComponentBeenAdded = false;
};

struct TTA_t {
    // List of reduced and unfolded components
    // list of unfolded symbols
};

#endif //MAVE_TTATYPES_H
