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
#ifndef MAVE_STRINGEXTENSIONS_H
#define MAVE_STRINGEXTENSIONS_H

std::vector<std::string> split(const std::string& s, char delimiter);
std::vector<std::string> split(const std::string& s, const std::string& delimiter);
// TODO: IFDEF WINDOWS default delimiter is '\\'
std::string GetFileNameOnly(const std::string& fullpath, char delimiter = '/');

void ltrim(std::string &s);
void rtrim(std::string &s);
void trim(std::string &s);
std::string ltrim_copy(std::string s);
std::string rtrim_copy(std::string s);
std::string trim_copy(std::string s);
bool contains(const std::string& s, const std::string& substring);

#endif //MAVE_STRINGEXTENSIONS_H
