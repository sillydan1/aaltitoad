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
#ifndef MAVE_CLICONFIG_H
#define MAVE_CLICONFIG_H
#include <mavepch.h>
#include <argvparse.h>

class CLIConfig {
public:
    static CLIConfig& getInstance() {
        static CLIConfig instance;
        return instance;
    }
private:
    CLIConfig();

    enum class option_requirement {
        REQUIRE = 0,
        OPTIONAL = 1
    };
    typedef std::vector<std::pair<option_requirement, option_t>> optionCollection_t;
    typedef std::map<std::string, argument_t> providedOptions_t;

    optionCollection_t cliOptions;
    providedOptions_t providedOptions;
    int status_code;
    void EnsureRequiredOptionsAreProvided();
    std::vector<option_t> GetCLIOptionsOnly();
public:
    CLIConfig(const CLIConfig&) = delete;
    void operator=(const CLIConfig&) = delete;

    void ParseCLIOptionsAndCheckForRequirements(int argc, char** argv);
    void PrintHelpMessage(const char* const* argv);
    int GetStatusCode() const { return status_code; }
    argument_t operator[](const std::string& lookup);

    bool isElementRequiredAndMissing(const std::pair<option_requirement, option_t> &el);
};

#endif //MAVE_CLICONFIG_H
