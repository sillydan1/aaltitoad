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
#ifndef MAVE_BASEJSONTYPEHANDLER_H
#define MAVE_BASEJSONTYPEHANDLER_H

typedef unsigned SizeType;
class BaseJsonTypeHandler {
public:
    bool virtual Null() { return default_value; }
    bool virtual Bool(bool b) { return default_value; }
    bool virtual Int(int i) { return default_value; }
    bool virtual Uint(unsigned u) { return default_value; }
    bool virtual Int64(int64_t i) { return default_value; }
    bool virtual Uint64(uint64_t u) { return default_value; }
    bool virtual Double(double d) { return default_value; }
    bool virtual RawNumber(const char* str, SizeType length, bool copy) { return default_value; }
    bool virtual String(const char* str, SizeType length, bool copy) { return default_value; }
    bool virtual StartObject() { return default_value; }
    bool virtual Key(const char* str, SizeType length, bool copy) { return default_value; }
    bool virtual EndObject(SizeType memberCount) { return default_value; }
    bool virtual StartArray() { return default_value; }
    bool virtual EndArray(SizeType elementCount) { return default_value; }

protected:
    bool default_value = true;
    void SetDefaultReturnValue(bool newval) { default_value = newval; }
};

#endif //MAVE_BASEJSONTYPEHANDLER_H
