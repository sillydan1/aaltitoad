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

#ifndef MAVE_TTATRACER_H
#define MAVE_TTATRACER_H
#include <mavepch.h>
#include <runtime/TTA.h>
#include <rapidjson/document.h>

/***
 * Can generate json style traces on Tick Tock Automata
 */
struct TTATracer {
    static void TraceSteps(const std::string& output_json_file, TTA& automata, unsigned int stepAmount);
private:
    /// Starts a new trace file.
    /// NB! Overrides if the file already exists.
    /// NB! Creates a new file if it doesn't already exist
    static std::ofstream OpenFile(const std::string& file);
    /// Appends the state to provided file
    static void AppendStateToFile(const TTA& state, std::ofstream& file);
    /// Closes up the file format
    static void CloseFile(std::ofstream& file);

    static void AppendStateVariablesToFile(const TTA& state, std::ofstream& file);
    static void AppendStateComponentsToFile(const TTA& state, std::ofstream& file);

    static rapidjson::Document ParseDocumentDOMStyle(const std::ifstream &file);
    static double GetTimerDelay(const unsigned int tick, const rapidjson::Document& doc);
};

#endif //MAVE_TTATRACER_H
