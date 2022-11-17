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
# TODO: Add license header to all source files
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
# TODO: Implement timers/clocks-support
CPMAddPackage(NAME expr GIT_TAG feature/tree_interpreter GITHUB_REPOSITORY sillydan1/expr OPTIONS "ENABLE_Z3 ON")
CPMAddPackage(NAME argvparse VERSION 1.2.3 GITHUB_REPOSITORY sillydan1/argvparse)
CPMAddPackage(NAME ctl-expr GIT_TAG main GITHUB_REPOSITORY sillydan1/ctl-expr)

CPMAddPackage("gh:yalibs/yatimer@1.0.0")
CPMAddPackage("gh:yalibs/yaoverload@1.0.0")
CPMAddPackage("gh:yalibs/yahashcombine@1.0.0")
CPMAddPackage("gh:yalibs/yathreadpool@1.0.1")
CPMAddPackage("gh:yalibs/yagraph@1.0.5")
CPMAddPackage("gh:yalibs/yatree@1.2.0")
CPMAddPackage("gh:yalibs/yauuid@1.0.1")
CPMAddPackage("gh:yalibs/yasetwrappers@1.0.0")
CPMAddPackage("gh:yalibs/yapermutation#main")

CPMAddPackage("gh:gabime/spdlog@1.8.2")
CPMAddPackage("gh:nlohmann/json@3.10.5")
CPMAddPackage("gh:neargye/magic_enum@0.8.1")

CPMAddPackage("gh:cpm-cmake/CPMLicenses.cmake@0.0.5")
cpm_licenses_create_disclaimer_target(write-licenses "${CMAKE_CURRENT_BINARY_DIR}/third_party.txt" "${CPM_PACKAGES}")

include_directories(
        ${CMAKE_BINARY_DIR}
        ${CMAKE_CURRENT_BINARY_DIR}
        ${argvparse_SOURCE_DIR}/include
        ${argvparse_SOURCE_DIR}/src
        ${rapidjson_SOURCE_DIR}/include
        ${ctl-expr_SOURCE_DIR}/include
        ${ctl-expr_SOURCE_DIR}/src
        ${ctl-expr_SOURCE_DIR}/../ctl-expr-build
        ${json_SOURCE_DIR}/include
        ${spdlog_SOURCE_DIR}/include
        ${expr_SOURCE_DIR}/include
        ${expr_SOURCE_DIR}/src
        ${expr_BUILD_DIR}
        ${optionparser_SOURCE_DIR}/src
        ${magic_enum_SOURCE_DIR}/include

        ${yatimer_SOURCE_DIR}/include
        ${yaoverload_SOURCE_DIR}/include
        ${yahashcombine_SOURCE_DIR}/include
        ${yathreadpool_SOURCE_DIR}/include
        ${yagraph_SOURCE_DIR}/include
        ${yatree_SOURCE_DIR}/include
        ${yauuid_SOURCE_DIR}/include
        ${yapermutation_SOURCE_DIR}/include
        ${yasetwrappers_SOURCE_DIR}/include
        src
)
set(${PROJECT_NAME}_SOURCES
        src/ntta/builder/ntta_builder.cpp
        src/ntta/builder/ntta_builder_2.cpp
        src/ntta/tta.cpp
        src/ntta/interesting_tocker.cpp
        src/plugin_system/plugin_system.cpp
        src/verification/forward_reachability.cpp
        src/verification/ctl_sat.cpp
        src/warnings.cpp
        src/extensions/random.cpp
        src/extensions/string_extensions.cpp)

add_library(${PROJECT_NAME} SHARED ${${PROJECT_NAME}_SOURCES})
if(${CODE_COVERAGE})
    target_link_options(${PROJECT_NAME} PUBLIC --coverage)
    target_compile_options(${PROJECT_NAME} PUBLIC --coverage)
endif()
target_link_libraries(${PROJECT_NAME} expr dl pthread ctl)
add_compile_definitions(ENABLE_Z3)

add_subdirectory(src/cli/simulator)
add_subdirectory(src/cli/verifier)
add_subdirectory(src/parser)
add_subdirectory(test)

if(NOT WIN32)
    target_precompile_headers(${PROJECT_NAME} PUBLIC src/aaltitoadpch.h)
endif()
