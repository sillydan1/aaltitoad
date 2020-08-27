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

#ifndef MAVE_MAVE_DEBUG_H
#define MAVE_MAVE_DEBUG_H

#ifndef NDEBUG
// Type debugger. Will fail compilation and provide you the full type of the template.
// Usage:
//          TD<decltype(my_var)> t;
//          TD<MyTemplate> t;
template<typename T> struct TD;

#endif // NDEBUG

#endif //MAVE_MAVE_DEBUG_H
