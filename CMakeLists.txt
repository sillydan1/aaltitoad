# aaltitoad - a verification engine for tick tock automata models
# Copyright (C) 2023 Asger Gitz-Johansen
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
cmake_minimum_required(VERSION 3.16) # 3.16+ because of target_precompiled_header
project(aaltitoad VERSION 1.1.0)
set(THREADS_PREFER_PTHREAD_FLAG ON)
set(CMAKE_CXX_STANDARD 20)
set(CXX_STANDARD_REQUIRED ON)
set(SPDLOG_BUILD_SHARED)
set(CONFIG_IN_FILE src/config.h.in)
set(CONFIG_OUT_FILE config.h)
include(cmake/CPM.cmake)
include(cmake/VER.cmake) # TODO: make a yalibs release of this cmake file
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_MACOSX_RPATH 1)
list(APPEND CMAKE_INSTALL_RPATH ${CMAKE_INSTALL_PREFIX}/lib)

find_package(BISON REQUIRED)
find_package(FLEX REQUIRED)

add_compile_definitions(DEFAULT_EXPRESSION_VALUE="true")
CPMAddPackage(NAME expr GIT_TAG feature/cpp-parsers GITHUB_REPOSITORY sillydan1/expr OPTIONS "ENABLE_Z3 ON")
CPMAddPackage("gh:sillydan1/argvparse@1.2.3")
CPMAddPackage("gh:sillydan1/ctl-expr#feat/cpp-parser")

CPMAddPackage("gh:yalibs/yatimer@1.0.0")
CPMAddPackage("gh:yalibs/yaoverload@1.0.0")
CPMAddPackage("gh:yalibs/yahashcombine@1.0.0")
CPMAddPackage("gh:yalibs/yathreadpool@1.0.1")
CPMAddPackage("gh:yalibs/yagraph@1.0.6")
CPMAddPackage("gh:yalibs/yatree@1.2.1")
CPMAddPackage("gh:yalibs/yauuid@1.0.1")
CPMAddPackage("gh:yalibs/yasetwrappers@1.0.0")
CPMAddPackage("gh:yalibs/yapermutation@1.1.2")

CPMAddPackage("gh:gabime/spdlog@1.8.2")
CPMAddPackage("gh:nlohmann/json@3.10.5")
CPMAddPackage("gh:neargye/magic_enum@0.8.1")

CPMAddPackage("gh:cpm-cmake/CPMLicenses.cmake@0.0.5")
cpm_licenses_create_disclaimer_target(write_licenses "${CMAKE_BINARY_DIR}/third_party.txt" "${CPM_PACKAGES}")

add_library(${PROJECT_NAME} SHARED 
        src/expr-wrappers/parameterized-expr-evaluator.cpp
        src/ntta/builder/ntta_builder.cpp
        src/ntta/tta.cpp
        src/ntta/interesting_tocker.cpp
        src/plugin_system/plugin_system.cpp
        src/verification/forward_reachability.cpp
        src/verification/ctl/ctl_sat.cpp
        src/util/warnings.cpp
        src/util/random.cpp
        src/util/string_extensions.cpp)
if(${CODE_COVERAGE})
    target_link_options(${PROJECT_NAME} PUBLIC --coverage)
    target_compile_options(${PROJECT_NAME} PUBLIC --coverage)
endif()
target_link_libraries(${PROJECT_NAME} expr_lang dl pthread ctl_lang)
target_include_directories(${PROJECT_NAME} PUBLIC
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
        # strictly not required, but it helps my clangd lsp setup and it doesn't hurt
        ${FLEX_INCLUDE_DIRS}
        ${BISON_INCLUDE_DIRS}
)

add_compile_definitions(ENABLE_Z3)
add_subdirectory(src/cli/simulator)
add_subdirectory(src/cli/verifier)
add_subdirectory(src/cli/detcheck)
add_subdirectory(src/parser)
add_subdirectory(test)

install(TARGETS ${PROJECT_NAME} expr_lang ctl_lang)
install(FILES src/man/tta.7 DESTINATION man/man7)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    install(FILES ${CMAKE_BINARY_DIR}/libz3.so DESTINATION lib)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    install(FILES ${CMAKE_BINARY_DIR}/libz3.dll DESTINATION lib)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    install(FILES ${CMAKE_BINARY_DIR}/libz3.dylib DESTINATION lib)
else()
    message(WARNING "unknown system ${CMAKE_SYSTEM_NAME}")
endif()

# TODO: determine required header files for plugins and add them to the install target
#       install(DIRECTORY src/ntta src/plugin_system src/util DESTINATION include FILES_MATCHING PATTERN "*.h")

if(NOT WIN32)
    target_precompile_headers(${PROJECT_NAME} PUBLIC src/aaltitoadpch.h)
endif()
