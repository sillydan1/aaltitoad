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
#include <random>
#include "random.h"

namespace aaltitoad::random {
    std::random_device r;
    auto value(int min, int max) -> int {
        std::default_random_engine e1(r());
        return std::uniform_int_distribution<int>(min,max)(e1);
    }

    auto value(double min, double max) -> double {
        std::default_random_engine e1(r());
        return std::uniform_real_distribution<double>(min, max)(e1);
    }
}

