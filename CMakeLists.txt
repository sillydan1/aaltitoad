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
project(aaltitoad VERSION 1.0.0)
include(cmake/CPM.cmake)
set(THREADS_PREFER_PTHREAD_FLAG ON)
set(CMAKE_CXX_STANDARD 20)
set(CXX_STANDARD_REQUIRED ON)
set(SPDLOG_BUILD_SHARED)

add_compile_definitions(DEFAULT_EXPRESSION_VALUE=true)
CPMAddPackage("gh:sillydan1/expr@1.2.0")
CPMAddPackage("gh:sillydan1/tinytimer@1.0.0")
CPMAddPackage("gh:sillydan1/overload#1.0.0")
CPMAddPackage("gh:sillydan1/ctlparser@1.0.0")
CPMAddPackage("gh:sillydan1/argvparse@1.2.2")
CPMAddPackage("gh:gabime/spdlog@1.8.2")
CPMAddPackage("gh:nlohmann/json@3.10.5")

configure_file(src/config.h.in config.h)
include_directories(
        ${CMAKE_BINARY_DIR}
        ${argvparse_SOURCE_DIR}/include
        ${overload_SOURCE_DIR}/include
        ${argvparse_SOURCE_DIR}/src # For some reason windows cant find files that are just right the hell there...
        ${rapidjson_SOURCE_DIR}/include
        ${ctlparser_SOURCE_DIR}/include
        ${json_SOURCE_DIR}/include
        ${spdlog_SOURCE_DIR}/include
        ${tinytimer_SOURCE_DIR}
        ${expr_SOURCE_DIR}/include
        ${expr_BUILD_DIR}
        ${optionparser_SOURCE_DIR}/src
        src
)
add_definitions(-DYYERROR_VERBOSE)
if (WIN32)
    add_definitions(-DYY_NO_UNISTD_H) # UNIX std does not exist on windows platforms
endif()

add_library(${PROJECT_NAME} SHARED
        src/runtime/ntta.cpp
        src/runtime/component.cpp
        src/runtime/edge.cpp
        src/plugin_system/plugin_system.cpp
        src/extensions/string_extensions.cpp)
add_dependencies(${PROJECT_NAME} expr)
target_link_libraries(${PROJECT_NAME} expr)

add_subdirectory(src/cli/simulator)
add_subdirectory(src/tocker_plugins)
add_subdirectory(src/parser_plugins)
add_subdirectory(test)

if(NOT WIN32)
    target_precompile_headers(${PROJECT_NAME} PUBLIC src/aaltitoadpch.h)
endif()

# install(TARGETS ${PROJECT_NAME})
