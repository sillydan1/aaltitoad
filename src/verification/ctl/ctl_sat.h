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
#ifndef AALTITOAD_CTL_SAT_H
#define AALTITOAD_CTL_SAT_H
#include <ctl_syntax_tree.h>
#include <ntta/tta.h>

namespace aaltitoad {
    auto is_satisfied(const ctl::syntax_tree_t& ast, const ntta_t& state) -> bool;
}

#endif //AALTITOAD_CTL_SAT_H
