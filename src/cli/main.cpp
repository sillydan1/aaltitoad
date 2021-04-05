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
#include <aaltitoadpch.h>
#include <runtime/TTA.h>
#include <model_parsers/TTAParser.h>
#include <cli/CLIConfig.h>
#include <verifier/trace_output/TTATracer.h>
#include <verifier/query_parsing/CTLQueryParser.h>
#include <verifier/ReachabilitySearcher.h>
#include <extensions/stringextensions.h>

int main(int argc, char** argv) {
    // Initialize CLI configuration (based on CLI Args)
    CLIConfig::getInstance().ParseCLIOptionsAndCheckForRequirements(argc, argv);
    auto& config = CLIConfig::getInstance();
    if(config.GetStatusCode() != EXIT_SUCCESS || config["help"]) {
        config.PrintHelpMessage(argv);
        return config.GetStatusCode();
    }
    if(config["version"]) {
        std::cout << GetFileNameOnly(argv[0]) << " version 0.9b" << std::endl;
        return 0;
    }
    if(config["verbosity"])
        spdlog::set_level(static_cast<spdlog::level::level_enum>(6-config["verbosity"].as_integer()));
    else
        spdlog::set_level(spdlog::level::level_enum::warn);

    // Parse the TTA
    TTAParser ttaParser{};
    TTA t = ttaParser.ParseFromFilePath(config["input"].as_string());

    // What nondeterminism strategy should we use
    auto strategy = nondeterminism_strategy_t::PICK_FIRST;
    if(config["nondeterminism-strategy"])
        strategy = (nondeterminism_strategy_t)config["nondeterminism-strategy"].as_integer();

    // Parse the queries
    if(config["query"]) {
        auto queryList = CTLQueryParser::ParseQueriesFile(config["query"].as_string(), t);
        ReachabilitySearcher s{queryList, t};
        bool allQueriesSatisfied = s.Search(strategy);
        return allQueriesSatisfied ? EXIT_SUCCESS : EXIT_FAILURE;
    }

    if(config["trace"]) {
        TTATracer::TraceSteps(config["trace-output"].as_string(), t, config["trace"].as_integer());
        return 0;
    }

    // Return the exit code.
    return config.GetStatusCode();
}
