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

#include "TTATracer.h"

void TTATracer::TraceSteps(const std::string &output_json_file, TTA &automata, unsigned int stepAmount) {
    auto file = OpenFile(output_json_file);
    for(int i = 0; i < stepAmount; i++) {
        AppendStateToFile(automata, file);
        if(i != stepAmount-1) file << ","; // Put commas after all states, except the last one.
        automata.Tick();
    }
    CloseFile(file);
}

std::ofstream TTATracer::OpenFile(const std::string &file) {
    std::ofstream outfile(file);
    outfile << "{ \"Ticks\" : [" << std::endl;
    return outfile;
}

void TTATracer::AppendStateToFile(const TTA &state, std::ofstream& file) {
    file << "{ \"Variables\" : [";
    AppendStateVariablesToFile(state, file);
    file << "], \"Components\" : [";
    AppendStateComponentsToFile(state, file);
    file << "] }";
}

void TTATracer::CloseFile(std::ofstream& file) {
    file << " ] }" << std::endl;
    file.close();
}

void TTATracer::AppendStateVariablesToFile(const TTA &state, std::ofstream &file) {
    auto& symbols = state.GetSymbols().map();
    int i = 0;
    for(auto& var : symbols) {
        // TODO: Re-sugaring
        if(var.second->type == STR)
            file << "{ \"" << var.first << "\" : " << var.second.str() << " }";
        else if(var.second->type == TIMER)
            file << "{ \"" << var.first << "\" : \"" << var.second.asInt() << "\" }";
        else
            file << "{ \"" << var.first << "\" : \"" << var.second.str() << "\" }";
        if(i++ != symbols.size()-1) file << ",\n";
    }
}

void TTATracer::AppendStateComponentsToFile(const TTA &state, std::ofstream &file) {
    auto& components = state.components;
    int i = 0;
    for(auto& component : components) {
        // TODO: Re-sugaring
        file << "{ \"" << component.first << "\" : \"" << component.second.currentLocation.identifier << "\" }";
        if(i++ != components.size()-1) file << ",\n";
    }
}
