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
        ntta/ntta_tests.cpp
        tta/tta_tests.cpp
        algorithms/tarjan_tests.cpp
        algorithms/dfs_tests.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE aaltitoad Catch2::Catch2WithMain yatta)
if(${COVERAGE_ENABLE})
    target_compile_options(${PROJECT_NAME} PRIVATE --coverage)
    target_link_options(${PROJECT_NAME} PRIVATE --coverage)
endif()