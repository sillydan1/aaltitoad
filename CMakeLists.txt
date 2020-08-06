# Copyright (C) 2020 Asger Gitz-Johansen
# This file is part of mave.
#
#  mave is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  mave is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with mave.  If not, see <https://www.gnu.org/licenses/>.

# 3.16+ because of "target_precompile_headers"
cmake_minimum_required(VERSION 3.16)
project(mave)

include_directories(
        dep/argvparse/include
        src
)

add_subdirectory(dep/argvparse/)

add_executable(mave
        src/cli/main.cpp
        src/cli/CLIConfig.cpp src/model_parsers/TTAParser.cpp)

target_link_libraries(mave
        argvparse
        )

target_precompile_headers(
        mave PUBLIC
        src/mavepch.h
)
