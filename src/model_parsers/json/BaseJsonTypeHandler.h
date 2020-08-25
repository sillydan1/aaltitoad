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
    virtual bool Null() { return default_value; }
    virtual bool Bool(bool b) { return default_value; }
    virtual bool Int(int i) { return default_value; }
    virtual bool Uint(unsigned u) { return default_value; }
    virtual bool Int64(int64_t i) { return default_value; }
    virtual bool Uint64(uint64_t u) { return default_value; }
    virtual bool Double(double d) { return default_value; }
    virtual bool RawNumber(const char* str, SizeType length, bool copy) { return default_value; }
    virtual bool String(const char* str, SizeType length, bool copy) { return default_value; }
    virtual bool StartObject() { return default_value; }
    virtual bool Key(const char* str, SizeType length, bool copy) { return default_value; }
    virtual bool EndObject(SizeType memberCount) { return default_value; }
    virtual bool StartArray() { return default_value; }
    virtual bool EndArray(SizeType elementCount) { return default_value; }

protected:
    bool default_value = true; // Should we fail (false) or succeed (true) by default?
    void SetDefaultReturnValue(bool newval) { default_value = newval; }
};

#endif //MAVE_BASEJSONTYPEHANDLER_H
