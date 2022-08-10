cmake_minimum_required(VERSION 3.21)
project(ctl)
find_package(FLEX REQUIRED)
find_package(BISON REQUIRED)

BISON_TARGET(ctl_parser ./grammar/ctl.y ${CMAKE_CURRENT_BINARY_DIR}/ctl.y.cpp)
FLEX_TARGET(ctl_scanner ./grammar/ctl.l ${CMAKE_CURRENT_BINARY_DIR}/ctl.l.cpp)
ADD_FLEX_BISON_DEPENDENCY(ctl_scanner ctl_parser)
add_library(ctl SHARED
        ${BISON_ctl_parser_OUTPUTS}
        ${FLEX_ctl_scanner_OUTPUTS})
add_executable(ctl_demo ./demo.cpp)
target_link_libraries(ctl_demo ctl argvparse)
