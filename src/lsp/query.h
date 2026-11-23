/**
 * aaltitoad - a verification engine for tick tock automata models
   Copyright (C) 2026 Asger Gitz-Johansen

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
#ifndef AALTITOAD_LSP_QUERY_H
#define AALTITOAD_LSP_QUERY_H
#include <parser/graphedit/network.h>
#include <verification/forward_reachability.h>
#include <string>

namespace aaltitoad::lsp {
    // Compile a query about a network (which must have been built) for the verifier's search: ctl-expr parses it, and
    // the verifier decides its kind and evaluates it, as for the queries of verifier(1). Only E F p and A G p are
    // answered. Throws std::invalid_argument with a message for the user for the others.
    //
    // aaltitoad-ls adds a notation to the language of ctl-expr, which is translated before it parses the query:
    // <instance>@<location> (e.g. "Main.lamp(1)@on") holds when that instance is in the location with that nickname or
    // id. The locations of ctl-expr hold when any instance is in them, and cannot be operands of expressions. The dot
    // cannot separate the two names, since "Main.lamp(1).on" is the private variable "on" of "Main.lamp(1)".
    auto compile_query(const std::string& text, const graphedit::network_t& network) -> forward_reachability_searcher::query_solution_t;
}

#endif //AALTITOAD_LSP_QUERY_H
