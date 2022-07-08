# Copyright (C) 2022 Asger Gitz-Johansen
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
set(THREADS_PREFER_PTHREAD_FLAG ON)
set(CMAKE_CXX_STANDARD 20)
set(CXX_STANDARD_REQUIRED ON)
set(SPDLOG_BUILD_SHARED)
set(CONFIG_IN_FILE src/config.h.in)
set(CONFIG_OUT_FILE config.h)
include(cmake/CPM.cmake)
include(cmake/VER.cmake)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

add_compile_definitions(DEFAULT_EXPRESSION_VALUE="true")
#CPMAddPackage("gh:sillydan1/expr@1.3.0")
CPMAddPackage("gh:sillydan1/expr#major/compilation")
CPMAddPackage("gh:sillydan1/overload#1.0.0")
CPMAddPackage("gh:sillydan1/ctlparser@1.0.0")
CPMAddPackage("gh:sillydan1/argvparse@1.2.2")
CPMAddPackage("gh:yalibs/yatimer@1.0.0")
CPMAddPackage("gh:yalibs/yathreadpool@1.0.1")
CPMAddPackage("gh:yalibs/yagraph@1.0.0")
CPMAddPackage("gh:gabime/spdlog@1.8.2")
CPMAddPackage("gh:nlohmann/json@3.10.5")

include_directories(
        ${CMAKE_BINARY_DIR}
        ${argvparse_SOURCE_DIR}/include
        ${argvparse_SOURCE_DIR}/src
        ${overload_SOURCE_DIR}/include
        ${rapidjson_SOURCE_DIR}/include
        ${ctlparser_SOURCE_DIR}/include
        ${json_SOURCE_DIR}/include
        ${spdlog_SOURCE_DIR}/include
        ${tinytimer_SOURCE_DIR}
        ${expr_SOURCE_DIR}/include
        ${expr_SOURCE_DIR}/src
        ${expr_BUILD_DIR}
        ${optionparser_SOURCE_DIR}/src
        ${yathreadpool_SOURCE_DIR}/include
        ${yatree_SOURCE_DIR}/include
        ${yagraph_SOURCE_DIR}/include
        src
)
set(${PROJECT_NAME}_SOURCES
        src/runtime/ntta.cpp
        src/runtime/component.cpp
        src/runtime/edge.cpp
        src/plugin_system/plugin_system.cpp
        src/extensions/string_extensions.cpp)

if(NOT DEFINED TEST_COVERAGE)
    add_library(${PROJECT_NAME} SHARED ${${PROJECT_NAME}_SOURCES})
    install(TARGETS ${PROJECT_NAME})
else()
    add_library(${PROJECT_NAME} ${${PROJECT_NAME}_SOURCES})
endif()
target_link_libraries(${PROJECT_NAME} expr dl pthread)

add_subdirectory(src/plugins)
add_subdirectory(src/cli/simulator)
add_subdirectory(test)

if(NOT WIN32)
    target_precompile_headers(${PROJECT_NAME} PUBLIC src/aaltitoadpch.h)
endif()
