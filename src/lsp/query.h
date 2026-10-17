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
#include <map>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace aaltitoad::lsp {
    // A query about a network (the queries of graphedit, docs/verification.md of graphedit), in the CTL of aaltitoad's
    // verifier: E F p (can a state be reached in which p holds?) or A G p (does p hold in every state that can be
    // reached?), where p is an expression over the variables of the network or a location id (the id of a vertex: an
    // instance of its template is in it). The UPPAAL notation E<> p and A[] p is understood as well.
    //
    // Locations can also be named in expressions as <instance>@<location>, e.g. "Main.lamp(1)@on": the instance (named
    // after its scope, as the debugger names it) is in the location with that nickname (or id). The dot cannot separate
    // them, since it separates the private variables of an instance ("Main.lamp(1).on" is the variable "on" of
    // "Main.lamp(1)").
    class query_t {
    public:
        enum class kind_t {
            // E F p: a state in which p holds proves it (a witness)
            reachability,
            // A G p: a state in which p does not hold disproves it (a counterexample)
            safety
        };

        // Compile a query about a network (which must have been built). Throws std::invalid_argument with a message for
        // the user if it is not a query that aaltitoad-ls answers.
        static auto compile(const std::string& text, const graphedit::network_t& network) -> query_t;

        auto kind() const -> kind_t;
        // Whether the predicate (p) holds in a state of the network. Throws if it cannot be evaluated.
        auto holds(const ntta_t& state) const -> bool;
        // Whether a state proves (reachability) or disproves (safety) the query
        auto decides(const ntta_t& state) const -> bool;

    private:
        // the instances and the locations in which a location of the predicate holds
        using places_t = std::vector<std::pair<std::string, std::string>>;

        kind_t kind_{kind_t::reachability};
        // the predicate: an expression, or a location id
        std::variant<expr::syntax_tree_t, std::string> predicate_{};
        // the identifiers that stand for the locations that the predicate names (<instance>@<location>)
        std::map<std::string, places_t> locations_{};
    };
}

#endif //AALTITOAD_LSP_QUERY_H
