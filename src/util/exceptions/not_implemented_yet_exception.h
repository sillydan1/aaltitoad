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
#ifndef AALTITOAD_NOT_IMPLEMENTED_YET_EXCEPTION_H
#define AALTITOAD_NOT_IMPLEMENTED_YET_EXCEPTION_H
#include "aaltitoadpch.h"

class not_implemented_yet_exception : public std::logic_error {
public:
    not_implemented_yet_exception () : std::logic_error{"function not yet implemented"} {}
};

#endif //AALTITOAD_NOT_IMPLEMENTED_YET_EXCEPTION_H
