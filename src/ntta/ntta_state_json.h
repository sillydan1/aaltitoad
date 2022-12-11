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
#ifndef AALTITOAD_NTTA_STATE_JSON_H
#define AALTITOAD_NTTA_STATE_JSON_H

namespace stream_mods {
    class ntta_state_json {};
    constexpr ntta_state_json json;
}

struct json_ostream {
    std::ostream& os;
};

inline json_ostream operator<<(std::ostream& os, stream_mods::ntta_state_json) {
    return { os };
}

template <typename T>
std::ostream& operator<<(json_ostream jos, const T& v) {
    return jos.os << v;
}

#endif //AALTITOAD_NTTA_STATE_JSON_H
