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
project(aaltitoad VERSION 0.9.2)
include(cmake/CPM.cmake)
set(THREADS_PREFER_PTHREAD_FLAG ON)
set(CMAKE_CXX_STANDARD 20)
set(CXX_STANDARD_REQUIRED ON)
set(SPDLOG_BUILD_SHARED)

CPMAddPackage("gh:sillydan1/expr#latest")
CPMAddPackage("gh:sillydan1/tinytimer@1.0.0")
CPMAddPackage("gh:sillydan1/overload#1.0.0")
CPMAddPackage("gh:sillydan1/argvparse@1.1.0")
CPMAddPackage("gh:sillydan1/ctlparser@1.0.0")
CPMAddPackage("gh:gabime/spdlog@1.8.2")
CPMAddPackage("gh:Tencent/rapidjson@1.1.0")
CPMAddPackage(
        NAME rapidjson
        GITHUB_REPOSITORY Tencent/rapidjson
        VERSION 1.1.0
        OPTIONS "RAPIDJSON_BUILD_DOC OFF" "RAPIDJSON_BUILD_EXAMPLES OFF" "RAPIDJSON_BUILD_TESTS OFF"
)
find_package(Threads REQUIRED)

configure_file(src/config.h.in config.h)
include_directories(
        ${CMAKE_BINARY_DIR}
        ${argvparse_SOURCE_DIR}/include
        ${argvparse_SOURCE_DIR}/src # For some reason windows cant find files that are just right the hell there...
        ${rapidjson_SOURCE_DIR}/include
        ${ctlparser_SOURCE_DIR}/include
        ${spdlog_SOURCE_DIR}/include
        ${tinytimer_SOURCE_DIR}
        ${expr_SOURCE_DIR}/include
        ${expr_BUILD_DIR}
        dep/cparse
        dep
        src
)

add_definitions(-DYYERROR_VERBOSE)
if (WIN32)
    add_definitions(-DYY_NO_UNISTD_H) # UNIX std does not exist on windows platforms
endif()

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

add_library(${PROJECT_NAME}_ntta
        src/runtime/tta/ntta.cpp
        src/runtime/tta/component.cpp
        src/runtime/tta/edge.cpp
)
add_dependencies(${PROJECT_NAME}_ntta expr)

add_executable(${PROJECT_NAME}
        src/cli/main.cpp
        src/cli/CLIConfig.cpp
        src/extensions/string_extensions.cpp
        src/runtime/tta.cpp
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
        Threads::Threads
        shunting_yard
        builtin_features)

if(NOT WIN32)
        target_precompile_headers(${PROJECT_NAME} PUBLIC src/aaltitoadpch.h)
endif()

install(TARGETS ${PROJECT_NAME})
