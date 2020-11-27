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
#include <mavepch.h>
#include <runtime/TTA.h>
#include <model_parsers/TTAParser.h>
#include "CLIConfig.h"
#include <verifier/trace_output/TTATracer.h>
#include <verifier/query_parsing/CTLQueryParser.h>

int main(int argc, char** argv) {
    // Initialize CLI configuration (based on CLI Args)
    CLIConfig::getInstance().ParseCLIOptionsAndCheckForRequirements(argc, argv);
    auto& config = CLIConfig::getInstance();
    if(config.GetStatusCode() != EXIT_SUCCESS || config["help"]) {
        config.PrintHelpMessage(argv);
        return config.GetStatusCode();
    }
    if(config["verbosity"])
        spdlog::set_level(static_cast<spdlog::level::level_enum>(5-config["verbosity"].as_integer()));
    else
        spdlog::set_level(spdlog::level::level_enum::warn);

    // Call the engine(s)
    TTAParser ttaParser{};
    TTA t = ttaParser.ParseFromFilePath(config["input"].as_string());

    if(config["query"]) {
        auto queryList = CTLQueryParser::ParseQueriesFile(config["query"].as_string(), t);
        for(auto& query : queryList) {
            spdlog::debug("Parsed Query:");
            query->tree_apply([](auto& n){ spdlog::debug("{0}: {1}", ConvertToString(n.type), n.token); });
            spdlog::debug("------------------");
        }
        return 0;
    }

    if(config["trace"]) {
        TTATracer::TraceSteps(config["trace-output"].as_string(), t, config["trace"].as_integer());
        return 0;
    }

    // Return the exit code.
    return config.GetStatusCode();
}
