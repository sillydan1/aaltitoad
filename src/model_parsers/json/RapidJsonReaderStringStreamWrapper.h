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
#ifndef MAVE_RAPIDJSONREADERSTRINGSTREAMWRAPPER_H
#define MAVE_RAPIDJSONREADERSTRINGSTREAMWRAPPER_H
#include <sstream>

/**
 * A simple wrapper for radpidjson's stream concept over an std::stringstream.
 * \Note: This class can ONLY read.
 * */
struct RapidJsonReaderStringStreamWrapper {
    typedef char Ch;

    RapidJsonReaderStringStreamWrapper(std::stringstream&& _stream) : stream{std::move(_stream)} {}

    Ch Peek() { return stream.peek(); }
    Ch Take() { return stream.get(); }
    size_t Tell() const { return static_cast<size_t>(stream.width()); }

    Ch* PutBegin() { return 0; }
    void Put(Ch) {  }
    void Flush() {  }
    size_t PutEnd(Ch* x) { return 0; }

    std::stringstream stream;
};

#endif //MAVE_RAPIDJSONREADERSTRINGSTREAMWRAPPER_H
