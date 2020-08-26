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
#ifndef MAVE_MAVEPCH_H
#define MAVE_MAVEPCH_H

// General purpose STL things
#include <iostream>
#include <algorithm>
#include <functional>
#include <memory>
#include <utility>
#include <fstream>
#include <sstream>
#include <filesystem>

// Data structures
#include <map>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <optional>

// Extra "big" libraries that are used everywhere
#include <spdlog/spdlog.h>

// Debug files
#ifndef NDEBUG
#include <debug/mave_debug.h>
#endif // NDEBUG

#endif //MAVE_MAVEPCH_H
