project(parser_plugins VERSION 1.0.0)
add_library(huppaal_parser SHARED
        scoped_template_builder/model.cpp
        scoped_template_builder/scoped_template_builder.cpp
        huppaal_parser.cpp)
target_link_libraries(huppaal_parser PUBLIC aaltitoad expr nlohmann_json::nlohmann_json)
if(${CODE_COVERAGE})
    target_link_options(huppaal_parser PUBLIC --coverage)
    target_compile_options(huppaal_parser PUBLIC --coverage)
endif()
