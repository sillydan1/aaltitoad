/**
 * aaltitoad - a verification engine for tick tock automata models
   Copyright (C) 2023 Asger Gitz-Johansen

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef AALTITOAD_GRAPHEDIT_PROBLEMS_H
#define AALTITOAD_GRAPHEDIT_PROBLEMS_H

// The identifiers of the problems that aaltitoad finds in graphedit models (the lint identifiers of its diagnostics).
// They are documented for users in the language server documentation of graphedit-ntta; keep them stable.
namespace aaltitoad::graphedit::problems {
    // No template is the main template (is_main)
    inline constexpr auto no_main_template = "AALTITOAD-E001";
    // Several templates are the main template
    inline constexpr auto several_main_templates = "AALTITOAD-E002";
    // Several models have the same name
    inline constexpr auto duplicate_names = "AALTITOAD-E003";
    // A template does not have exactly one initial and one final location
    inline constexpr auto initial_and_final_locations = "AALTITOAD-E004";
    // A connection leads into or out of a subcomponent
    inline constexpr auto subcomponent_connection = "AALTITOAD-E005";
    // An edge does not arrive at a location (a nail leads nowhere, to several vertices or in a circle)
    inline constexpr auto broken_edge = "AALTITOAD-E006";
    // aaltitoad cannot parse an expression (a guard, an update, declarations or the arguments of an invocation)
    inline constexpr auto invalid_expression = "AALTITOAD-E007";
    // A template has extern or persistent declarations
    inline constexpr auto unsupported_declaration = "AALTITOAD-E008";
    // A subcomponent names a template that does not exist
    inline constexpr auto unknown_template = "AALTITOAD-E009";
    // The arguments of a subcomponent do not match the parameters of its template
    inline constexpr auto argument_mismatch = "AALTITOAD-E010";
    // Templates instantiate each other in a circle
    inline constexpr auto recursive_instantiation = "AALTITOAD-E011";
    // An expression uses a variable that is not declared
    inline constexpr auto undeclared_identifier = "AALTITOAD-E012";
    // aaltitoad cannot build the network for another reason
    inline constexpr auto build_failed = "AALTITOAD-E013";
    // A model file that aaltitoad cannot read (e.g. of graphedit 1.x)
    inline constexpr auto unreadable_model = "AALTITOAD-W001";
}

#endif //AALTITOAD_GRAPHEDIT_PROBLEMS_H
