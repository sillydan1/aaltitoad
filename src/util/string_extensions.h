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
#ifndef AALTITOAD_STRINGEXTENSIONS_H
#define AALTITOAD_STRINGEXTENSIONS_H
#include <regex>
#include <string>
#include <vector>

auto split(const std::string& s, char delimiter) -> std::vector<std::string>;
auto split(const std::string& s, const std::string& delimiter) -> std::vector<std::string>;
auto regex_split(const std::string& s, const std::string& regex) -> std::vector<std::string>;
auto join(const std::string& separator, const std::vector<std::string>& s) -> std::string;

void ltrim(std::string &s);
void rtrim(std::string &s);
void trim(std::string &s);
auto ltrim_copy(std::string s) -> std::string;
auto rtrim_copy(std::string s) -> std::string;
auto trim_copy(std::string s) -> std::string;
bool contains(const std::string& s, const std::string& substring);
auto containsString(const std::string& s, const std::string& substring) -> std::optional<const size_t>;
auto regex_replace_all(const std::string& original, const std::regex& reg, const std::string& replacement) -> std::string;
void lower_case(std::string& s);
auto lower_case(const std::string& s) -> std::string;
void kebab_case(std::string& s);
auto kebab_case(const std::string& s) -> std::string;

struct string_builder {
    std::stringstream ss;
    template<typename T>
    auto operator<<(const T &data) -> string_builder& {
        ss << data;
        return *this;
    }
    operator std::string() const { return ss.str(); } // NOLINT(google-explicit-constructor)
};

auto string_format(const std::string &fmt, ...) -> std::string;


#endif //AALTITOAD_STRINGEXTENSIONS_H
