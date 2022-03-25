project(tocker_plugins VERSION 1.0.0)
include_directories(../src/runtime)
add_library(pipe_tocker SHARED
        pipe_tocker.cpp)
target_link_libraries(pipe_tocker aaltitoad)
