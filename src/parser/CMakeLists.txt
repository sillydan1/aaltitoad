project(parser_plugins VERSION 1.0.0)
add_library(huppaal_parser SHARED huppaal_parser.cpp)
target_link_libraries(huppaal_parser PUBLIC aaltitoad expr)
