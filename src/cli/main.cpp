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
#include <iostream>
#include "CLIConfig.h"

int main(int argc, char** argv) {
    // Initialize CLI configuration (based on CLI Args)
    CLIConfig config{};
    config.ParseCLIOptionsAndCheckForRequirements(argc, argv);
    if(config.GetStatusCode() != EXIT_SUCCESS) {
        config.PrintHelpMessage(argv);
        return config.GetStatusCode();
    }
    // Call the engine(s)

    // Return the exit code.
    return config.GetStatusCode();
}
