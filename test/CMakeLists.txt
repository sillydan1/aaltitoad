# aaltitoad - a verification engine for tick tock automata models
# Copyright (C) 2026 Asger Gitz-Johansen
#
#   This program is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program.  If not, see <https://www.gnu.org/licenses/>.
cmake_minimum_required(VERSION 3.16)
project(aaltitoad_tests)
option(COVERAGE_ENABLE OFF)
set(THREADS_PREFER_PTHREAD_FLAG ON)
include(../cmake/CPM.cmake)
CPMAddPackage("gh:catchorg/Catch2@3.5.4")
find_package(Catch2 3 REQUIRED)
add_compile_definitions(AALTITOAD_PROJECT_DIR="${CMAKE_SOURCE_DIR}")
add_executable(${PROJECT_NAME}
        tta/tta_tests.cpp
        tta/tocker_tests.cpp
        verification/parser_tests.cpp
        parser/scoped_template_builder_tests.cpp
        graphedit/ntta_model_tests.cpp
        graphedit/network_tests.cpp
        debugger/runtime_tests.cpp
        lsp/project_tests.cpp
        lsp/language_server_tests.cpp
        verification/forward_reachability_tests.cpp
        algorithms/tarjan_tests.cpp
        plugin_system/plugin_system_tests.cpp)
target_link_libraries(${PROJECT_NAME} PUBLIC aaltitoad hawk_parser graphedit_parser aaltitoad_language_server Catch2::Catch2WithMain)
if(${CODE_COVERAGE})
    target_link_options(${PROJECT_NAME} PUBLIC --coverage)
    target_compile_options(${PROJECT_NAME} PUBLIC --coverage)
endif()
# the end-to-end tests run the language server program
target_compile_definitions(${PROJECT_NAME} PRIVATE AALTITOAD_LS_PROGRAM="$<TARGET_FILE:aaltitoad-ls>")
add_dependencies(${PROJECT_NAME} aaltitoad-ls)
# the plugin system tests load a plugin (the hawk parser) and a shared library that is not a plugin
target_compile_definitions(${PROJECT_NAME} PRIVATE
        AALTITOAD_HAWK_PARSER_PLUGIN="$<TARGET_FILE:hawk_parser>"
        AALTITOAD_NON_PLUGIN_LIBRARY="$<TARGET_FILE:aaltitoad>")
