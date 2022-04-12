project(default_plugins VERSION 1.0.0)
include_directories(../runtime)
add_definitions(-DPLUGIN_VERSION="v${PROJECT_VERSION}")

add_library(h-uppaal-parser SHARED parsers/h-uppaal-parser.cpp)
target_link_libraries(h-uppaal-parser aaltitoad)

add_library(hawk-parser SHARED parsers/hawk-parser.cpp)
target_link_libraries(hawk-parser aaltitoad)

add_library(pipe_tocker SHARED tockers/pipe_tocker.cpp)
target_link_libraries(pipe_tocker aaltitoad)

add_custom_target(${PROJECT_NAME} DEPENDS h-uppaal-parser hawk-parser pipe_tocker)
