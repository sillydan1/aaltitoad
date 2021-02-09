# Copyright (C) 2020 Asger Gitz-Johansen
# This file is part of aaltitoad.
#
#  aaltitoad is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  aaltitoad is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with aaltitoad.  If not, see <https://www.gnu.org/licenses/>.

# 3.16+ because of target_precompiled_header
cmake_minimum_required(VERSION 3.16)
project(aaltitoad)

include_directories(
        dep/argvparse/include 
        dep/argvparse/src # For some reason windows cant find files that are just right the hell there...
        dep/rapidjson/include
        dep/spdlog/include
        dep/cparse
        dep
        src
)

set (CMAKE_CXX_STANDARD 17)
set (CXX_STANDARD_REQUIRED ON)
set (SPDLOG_BUILD_SHARED)
set (CMAKE_CXX_STANDARD_LIBRARIES -lstdc++fs)
add_compile_options(-lpthread)
add_compile_options(-lstdc++fs)
add_definitions(-DYYERROR_VERBOSE)
if (WIN32)
    add_definitions(-DYY_NO_UNISTD_H) # UNIX std does not exist on windows platforms
endif()

# Third party libraries, compiled with CMAKE
add_subdirectory(dep/argvparse/)
add_subdirectory(dep/spdlog)
add_subdirectory(dep/ctlparser)

# Third party libraries, compiled with something else
####
add_library(shunting_yard OBJECT
        dep/cparse/shunting-yard.cpp
        dep/cparse/packToken.cpp
        dep/cparse/functions.cpp
        dep/cparse/containers.cpp)
add_library(builtin_features OBJECT
        dep/cparse/builtin-features.cpp)
target_link_libraries(builtin_features PUBLIC shunting_yard)
####

add_executable(${PROJECT_NAME}
        src/cli/main.cpp
        src/cli/CLIConfig.cpp
        src/extensions/stringextensions.cpp
        src/runtime/TTA.cpp
        src/runtime/UpdateExpression.cpp
        src/verifier/query_parsing/CTLQueryParser.cpp
        src/verifier/trace_output/TTATracer.cpp
        src/verifier/trace_output/TTAResugarizer.cpp
        src/json_parsing/JSONParser.cpp
        src/verifier/ReachabilitySearcher.cpp
        src/runtime/VariablePredicate.cpp
        src/verifier/TTASuccessorGenerator.cpp
        src/extensions/tree_extensions.cpp
        src/model_parsers/TTAParser.cpp
        src/model_parsers/TTATypes.cpp
        )

target_link_libraries(${PROJECT_NAME}
        argvparse
        spdlog
        ctlparser

        shunting_yard
        builtin_features)

target_precompile_headers(${PROJECT_NAME} PUBLIC src/aaltitoadpch.h)

install(TARGETS ${PROJECT_NAME})
