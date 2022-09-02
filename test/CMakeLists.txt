cmake_minimum_required(VERSION 3.16)
project(aaltitoad_tests)
option(COVERAGE_ENABLE OFF)
set(THREADS_PREFER_PTHREAD_FLAG ON)
set(CMAKE_CXX_STANDARD 20)
set(CXX_STANDARD_REQUIRED ON)
include(../cmake/CPM.cmake)
CPMAddPackage("gh:catchorg/Catch2@3.0.0-preview4")
find_package(Catch2 3 REQUIRED)
add_executable(${PROJECT_NAME}
        tta/tta_tests.cpp
        tta/tocker_tests.cpp
        verification/parser_tests.cpp
        verification/forward_reachability_tests.cpp
        algorithms/tarjan_tests.cpp)
target_link_libraries(${PROJECT_NAME} PUBLIC aaltitoad huppaal_parser Catch2::Catch2WithMain)
if(${CODE_COVERAGE})
    target_link_options(${PROJECT_NAME} PUBLIC --coverage)
    target_compile_options(${PROJECT_NAME} PUBLIC --coverage)
endif()
