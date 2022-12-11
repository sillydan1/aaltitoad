# aaltitoad - a verification engine for tick tock automata models
#   Copyright (C) 2023 Asger Gitz-Johansen
#
#    This program is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program.  If not, see <https://www.gnu.org/licenses/>.
#
project(parser_plugins VERSION 1.0.0)
add_library(hawk_parser SHARED
        hawk/scoped_template_builder/model.cpp
        hawk/scoped_template_builder/scoped_template_builder.cpp
        hawk/scoped_template_builder/scoped_interpreter.cpp
        hawk/hawk_parser.cpp)
target_link_libraries(hawk_parser PUBLIC aaltitoad expr nlohmann_json::nlohmann_json)
if(${CODE_COVERAGE})
    target_link_options(hawk_parser PUBLIC --coverage)
    target_compile_options(hawk_parser PUBLIC --coverage)
endif()
