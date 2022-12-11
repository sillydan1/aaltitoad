/**
 * aaltitoad - a verification engine for tick tock automata models
   Copyright (C) 2023 Asger Gitz-Johansen

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
//// PCH = Precompiled Header.
//// This speeds up compile time by ~75%.
//// Dont overdo it though. Only put things here that are being used (included) across the entire codebase
#ifndef AALTITOAD_AALTITOADPCH_H
#define AALTITOAD_AALTITOADPCH_H

// General purpose STL things
#include <iostream>
#include <algorithm>
#include <functional>
#include <memory>
#include <utility>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <type_traits>
#include <map>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <optional>
#include <variant>

// Extra "big" libraries that are used everywhere
#include <spdlog/spdlog.h>

// Other
#include <util/string_extensions.h>

// Debug files
#ifndef NDEBUG
#endif // NDEBUG

#endif
