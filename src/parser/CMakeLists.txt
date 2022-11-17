project(parser_plugins VERSION 1.0.0)
add_library(hawk_parser SHARED
        hawk/scoped_template_builder/model.cpp
        hawk/scoped_template_builder/scoped_template_builder.cpp
        hawk/hawk_parser.cpp)
target_link_libraries(hawk_parser PUBLIC aaltitoad expr nlohmann_json::nlohmann_json)
if(${CODE_COVERAGE})
    target_link_options(hawk_parser PUBLIC --coverage)
    target_compile_options(hawk_parser PUBLIC --coverage)
endif()
