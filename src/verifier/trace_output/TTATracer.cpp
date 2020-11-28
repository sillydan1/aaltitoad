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
#include <json_parsing/JSONParser.h>
#include "TTATracer.h"
#include "TTAResugarizer.h"

void TTATracer::TraceSteps(const std::string &output_json_file, TTA &automata, unsigned int stepAmount) {
    auto file = OpenFile(output_json_file);
    std::optional<rapidjson::Document> timinginfo = {};
    if(CLIConfig::getInstance()["timing-info"]) {
        auto timingfilename = CLIConfig::getInstance()["timing-info"].as_string();
        std::ifstream timingfile{timingfilename};
        if (timingfile) timinginfo = ParseDocumentDOMStyle(timingfile);
        else spdlog::error("Unable to open timing information file '{0}'", timingfilename);
    }
    auto nondetstrat = nondeterminism_strategy_t::PANIC;
    if(CLIConfig::getInstance()["nondeterminism-strategy"])
        nondetstrat = static_cast<nondeterminism_strategy_t>(CLIConfig::getInstance()["nondeterminism-strategy"].as_integer());
    for(int i = 0; i < stepAmount; i++) {
        AppendStateToFile(automata, file);
        if(i != stepAmount-1) file << ","; // Put commas after all states, except the last one.

        auto state = automata.GetNextTickStates(nondetstrat);

        if(CLIConfig::getInstance()["timing-info"] && timinginfo.has_value())
            automata.DelayAllTimers(GetTimerDelay(i, timinginfo.value()));

        automata.SetCurrentState(state[0]);
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
        if(var.second->type == STR)
            file << "{ \"" << TTAResugarizer::Resugar(var.first) << "\" : " << var.second.str() << " }";
        else if(var.second->type == TIMER)
            file << "{ \"" << TTAResugarizer::Resugar(var.first) << "\" : \"" << var.second.asDouble() << "\" }";
        else
            file << "{ \"" << TTAResugarizer::Resugar(var.first) << "\" : \"" << var.second.str() << "\" }";
        if(i++ != symbols.size()-1) file << ",\n";
    }
}

void TTATracer::AppendStateComponentsToFile(const TTA &state, std::ofstream &file) {
    auto& components = state.components;
    int i = 0;
    for(auto& component : components) {
        // TODO: Re-sugaring
        auto resugaredName = TTAResugarizer::Resugar(component.first);
        auto resugaredValue = TTAResugarizer::Resugar(component.second.currentLocation.identifier);
        file << "{ \"" << resugaredName << "\" : \"" << resugaredValue.substr(resugaredName.size()+1) << "\" }";
        if(i++ != components.size()-1) file << ",\n";
    }
}

rapidjson::Document TTATracer::ParseDocumentDOMStyle(const std::ifstream &file) {
    // TODO: DOM Style can be slow and rapidjson provides faster parsing strategies. Extend when it becomes a problem
    std::stringstream filestream{};
    filestream << file.rdbuf();
    rapidjson::Document d;
    d.Parse(filestream.str().c_str());
    return d;
}

double TTATracer::GetTimerDelay(const unsigned int tick, const rapidjson::Document &doc) {
    std::stringstream ss; ss << tick;
    if(JSONParser::DoesMemberExistAndIsInt(doc, ss.str()))
        return doc[ss.str().c_str()].GetInt();
    else spdlog::error("Timing information file does not contain information from tick '{0}'. Defaulting to zero delay", ss.str());
    return 0;
}
