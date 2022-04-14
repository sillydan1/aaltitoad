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
#ifndef AALTITOAD_STRINGEXTENSIONS_H
#define AALTITOAD_STRINGEXTENSIONS_H

std::vector<std::string> split(const std::string& s, char delimiter);
std::vector<std::string> split(const std::string& s, const std::string& delimiter);
std::vector<std::string> regex_split(const std::string& s, const std::string& regex);

void ltrim(std::string &s);
void rtrim(std::string &s);
void trim(std::string &s);
std::string ltrim_copy(std::string s);
std::string rtrim_copy(std::string s);
std::string trim_copy(std::string s);
bool contains(const std::string& s, const std::string& substring);
std::optional<const size_t> containsString(const std::string& s, const std::string& substring);

struct string_builder {
    std::stringstream ss;
    template<typename T>
    auto operator<<(const T &data) -> string_builder& {
        ss << data;
        return *this;
    }
    operator std::string() const { return ss.str(); } // NOLINT(google-explicit-constructor)
};

#endif //AALTITOAD_STRINGEXTENSIONS_H
