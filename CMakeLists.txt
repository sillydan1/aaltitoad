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
        dep/rapidjson/include
        dep/spdlog/include
        src
)

set (CMAKE_CXX_STANDARD 17)
set (SPDLOG_BUILD_SHARED)
set (CMAKE_CXX_STANDARD_LIBRARIES -lstdc++fs)

add_subdirectory(dep/argvparse/)
add_subdirectory(dep/spdlog)

add_library(ttaparse
        src/model_parsers/TTAParser.cpp
        src/model_parsers/TTATypes.cpp)

add_executable(mave
        src/cli/main.cpp
        src/cli/CLIConfig.cpp
        src/extensions/stringextensions.cpp
        src/runtime/TTA.cpp)

target_link_libraries(mave
        argvparse
        spdlog
        ttaparse)

target_precompile_headers(mave PUBLIC src/mavepch.h)
target_precompile_headers(ttaparse PUBLIC src/mavepch.h)
