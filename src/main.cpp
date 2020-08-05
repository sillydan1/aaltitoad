/**
 * Copyright (C) 2020 Asger Gitz-Johansen

   This file is part of mave.

    mave is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    mave is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with mave.  If not, see <https://www.gnu.org/licenses/>.
 */
#include <argvparse.h>
#include <iostream>

int print_help_message(const char* const* argv, int return_val = EXIT_SUCCESS);
std::vector<option_t> my_cli_options = {
        {"input",  'i', argument_requirement::REQUIRE_ARG ,"Input file"},
        {"output", 'o', argument_requirement::REQUIRE_ARG, "Output file. Will be created, if not already exists"},
        {"in-type",'n', argument_requirement::REQUIRE_ARG, "The type of input modelling language"},
        {"out-type",'u', argument_requirement::REQUIRE_ARG,"The type of output modelling language"}
};

int main(int argc, char** argv) {
    // Then call get_arguments
    auto cli_arguments = get_arguments(my_cli_options, argc, argv);
    // If the help flag was provided, print out the help message
    if(cli_arguments["help"])
        return print_help_message(argv);

    if(cli_arguments["input"]) {

    } else
        return print_help_message(argv, EXIT_FAILURE);

    return EXIT_SUCCESS;
}

int print_help_message(const char* const* argv, int return_val) {
    std::cout << argv[0] << " is a program that translates Modelling languages to other modelling languages."
                            " Below are the possible options.\nCopyright (C) 2020  Asger Gitz-Johansen\n"
                            "\n"
                            "    This program is free software: you can redistribute it and/or modify\n"
                            "    it under the terms of the GNU General Public License as published by\n"
                            "    the Free Software Foundation, either version 3 of the License, or\n"
                            "    (at your option) any later version.\n"
                            "\n"
                            "    This program is distributed in the hope that it will be useful,\n"
                            "    but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
                            "    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
                            "    GNU General Public License for more details.\n"
                            "\n"
                            "    You should have received a copy of the GNU General Public License\n"
                            "    along with this program.  If not, see <https://www.gnu.org/licenses/>.\n";
    print_argument_help(my_cli_options);
    return return_val;
}
