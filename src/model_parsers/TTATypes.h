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
#ifndef MAVE_TTATYPES_H
#define MAVE_TTATYPES_H
#include <aaltitoadpch.h>
#include "runtime/tta.h"

struct TTAIR_t {
public:
    struct Location {
        bool isImmediate;
        std::string identifier;
    };
    struct Edge {
        Location sourceLocation;
        Location targetLocation;
        std::string guardExpression;
        std::string updateExpression;
    };
    struct Symbol {
        std::string identifier;
        TTASymbol_t value;
    };
    struct Component {
        std::string name;
        Location initialLocation;
        Location endLocation;
        bool isMain = false;
        std::vector<Edge> edges = {};
        std::vector<Symbol> symbols = {};
    };
public:
    std::vector<Component> components = {};
    std::vector<Symbol> externalSymbols = {};
    std::vector<Symbol> internalSymbols = {};

public:
    [[nodiscard]] std::optional<std::vector<Component>::const_iterator> FindMainComponent() const;
    void AddComponent(Component&& component);
    inline void AddExternalSymbols(std::vector<Symbol>&& _externalSymbols) {
        externalSymbols = std::move(_externalSymbols);
    }
    inline void AddInternalSymbols(std::vector<Symbol>&& _internalSymbols) {
        internalSymbols = std::move(_internalSymbols);
    }

private:
    bool hasMainComponentBeenAdded = false;
};

#endif //MAVE_TTATYPES_H
