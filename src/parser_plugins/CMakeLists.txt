project(parser_plugins VERSION 1.0.0)
include_directories(../runtime)
add_library(h-uppaal-parser SHARED
        h-uppaal-parser.cpp)
target_link_libraries(h-uppaal-parser aaltitoad)
