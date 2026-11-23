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
#include "query.h"
#include <expr-wrappers/ctl-interpreter.h>
#include <verification/ctl/ctl_sat.h>
#include <algorithm>
#include <cctype>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <utility>
#include <vector>

namespace aaltitoad::lsp {
    namespace {
        // the instances and the locations in which a named location holds
        using places_t = std::vector<std::pair<std::string, std::string>>;

        // Captures what is written to std::cerr while it lives (the parsers of aaltitoad report syntax errors there)
        class cerr_capture {
        public:
            cerr_capture() : previous{std::cerr.rdbuf(captured.rdbuf())} {}
            cerr_capture(const cerr_capture&) = delete;
            auto operator=(const cerr_capture&) -> cerr_capture& = delete;
            ~cerr_capture() { std::cerr.rdbuf(previous); }
            auto text() const -> std::string { return captured.str(); }
        private:
            std::stringstream captured{};
            std::streambuf* previous;
        };

        auto fail(const std::string& message) -> std::invalid_argument {
            return std::invalid_argument{message};
        }

        auto is_name_character(char c) -> bool {
            return std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '.';
        }

        auto is_location_character(char c) -> bool {
            return std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '-';
        }

        // The UPPAAL notation of the quantifiers (E<>, A[]) in the notation of aaltitoad (E F, A G)
        auto with_aaltitoad_quantifiers(std::string text) -> std::string {
            auto start = text.find_first_not_of(" \t\r\n");
            if(start == std::string::npos)
                return text;
            if(text.compare(start, 3, "E<>") == 0)
                text.replace(start, 3, "E F ");
            else if(text.compare(start, 3, "A[]") == 0)
                text.replace(start, 3, "A G ");
            return text;
        }

        // A location that a query names (<instance>@<location>)
        struct named_location_t {
            std::string instance{};
            std::string location{};
        };

        // The query with the locations that it names (outside of string literals) replaced by identifiers
        auto with_location_identifiers(const std::string& text, std::map<std::string, named_location_t>& named) -> std::string {
            std::string result{};
            for(std::size_t i = 0; i < text.size();) {
                if(text[i] == '"') {
                    auto end = i + 1;
                    while(end < text.size() && text[end] != '"')
                        end += text[end] == '\\' ? 2 : 1;
                    end = std::min(end + 1, text.size());
                    result += text.substr(i, end - i);
                    i = end;
                    continue;
                }
                if(text[i] != '@') {
                    result += text[i++];
                    continue;
                }
                // the instance is the name before the @ (which was copied already), the location the name after it.
                // NOTE: parentheses belong to the name when they are balanced (e.g. "(Main.lamp(1)@on" is "Main.lamp(1)")
                auto instance_start = result.size();
                auto depth = 0;
                while(instance_start > 0) {
                    auto c = result[instance_start - 1];
                    if(c == ')')
                        depth++;
                    else if(c == '(' && depth == 0)
                        break;
                    else if(c == '(')
                        depth--;
                    else if(!is_name_character(c) && !(depth > 0 && (c == ',' || c == ' ')))
                        break;
                    instance_start--;
                }
                if(depth != 0)
                    throw fail("the parentheses of the instance before the @ do not match");
                auto location_end = i + 1;
                while(location_end < text.size() && is_location_character(text[location_end]))
                    location_end++;
                auto instance = result.substr(instance_start);
                auto location = text.substr(i + 1, location_end - i - 1);
                if(instance.empty() || location.empty())
                    throw fail("a location is named <instance>@<location> (e.g. Main.lamp(1)@on)");
                auto identifier = "__location_" + std::to_string(named.size());
                named[identifier] = {instance, location};
                result.replace(instance_start, std::string::npos, identifier);
                i = location_end;
            }
            return result;
        }

        // The locations of an instance with a nickname or an id
        auto locations_of(const graphedit::network_t& network, const named_location_t& named) -> places_t {
            auto instance = network.instances.find(named.instance);
            if(instance == network.instances.end()) {
                std::string names{};
                std::size_t count = 0;
                for(auto& [name, unused] : network.instances)
                    if(count++ < 5)
                        names += (names.empty() ? "" : ", ") + name;
                if(network.instances.size() > 5)
                    names += ", ...";
                throw fail("the network has no instance '" + named.instance + "' (instances are named after their scope: " + names + ")");
            }
            places_t result{};
            for(auto& location : network.models.at(instance->second.model).locations)
                if(location.id == named.location || location.nickname == named.location)
                    result.emplace_back(named.instance, location.id);
            if(result.empty())
                throw fail("'" + named.instance + "' has no location '" + named.location + "' (a nickname or an id)");
            return result;
        }

        auto node_of(const ctl::syntax_tree_t& tree) -> const ctl::underlying_syntax_node_t& {
            return tree.node;
        }

        // The predicate p of E F p or A G p, as ctl-expr parsed it (root, modal, quantifier, p)
        auto predicate_of(const ctl::syntax_tree_t& tree) -> const ctl::syntax_tree_t& {
            const auto& query = tree.children().at(0);
            const auto& path = query.children().at(0);
            const auto* modal = std::get_if<ctl::modal_t>(&node_of(query));
            const auto* quantifier = std::get_if<ctl::quantifier_t>(&node_of(path));
            auto answered = modal != nullptr && quantifier != nullptr &&
                    ((modal->operator_type == ctl::modal_op_t::E && quantifier->operator_type == ctl::quantifier_op_t::F) ||
                     (modal->operator_type == ctl::modal_op_t::A && quantifier->operator_type == ctl::quantifier_op_t::G));
            if(!answered)
                throw fail("aaltitoad-ls answers E F p (can p become true?) and A G p (does p always hold?)");
            const auto& predicate = path.children().at(0);
            // NOTE: ctl-expr builds the operators between queries as expressions with the queries as children, which
            //       aaltitoad::is_satisfied cannot evaluate (it aborts)
            if(!predicate.children().empty())
                throw fail("aaltitoad-ls does not answer queries in queries: the predicate is an expression or a location");
            return predicate;
        }

        // Evaluates a query that names locations as the verifier does, with their identifiers bound to whether they hold
        auto with_bound_locations(ctl::syntax_tree_t tree, std::map<std::string, places_t> locations) -> forward_reachability_searcher::predicate_t {
            return [tree = std::move(tree), locations = std::move(locations)](const ntta_t& state) {
                auto bound = state;
                for(auto& [identifier, places] : locations)
                    bound.symbols[identifier] = expr::symbol_value_t{
                            std::any_of(places.begin(), places.end(), [&state](const auto& place) {
                                return state.components.at(place.first).current_location->first == place.second;
                            })};
                return aaltitoad::is_satisfied(tree, bound);
            };
        }
    }

    auto compile_query(const std::string& text, const graphedit::network_t& network) -> forward_reachability_searcher::query_solution_t {
        if(!network.automata)
            throw fail("aaltitoad cannot build the network");
        const auto& automata = *network.automata;
        std::map<std::string, named_location_t> named{};
        auto translated = with_location_identifiers(with_aaltitoad_quantifiers(text), named);

        // the query in the CTL of aaltitoad
        ctl::syntax_tree_t tree{};
        {
            cerr_capture capture{};
            try {
                auto environment = automata.symbols;
                tree = ctl_interpreter{environment}.compile(translated);
            } catch(const std::exception& e) {
                auto reported = capture.text();
                auto line = reported.substr(0, reported.find('\n'));
                throw fail("the query is not valid: " + (line.empty() ? std::string{e.what()} : line) +
                           " (queries are E F p or A G p, where p is an expression or a location)");
            }
        }

        const auto& predicate = predicate_of(tree);
        std::map<std::string, places_t> locations{};
        for(auto& [identifier, location] : named)
            locations[identifier] = locations_of(network, location);

        if(const auto* location = std::get_if<ctl::location_t>(&node_of(predicate))) {
            // a location id of the grammar of ctl-expr: some instance is in it
            auto found = std::any_of(network.models.begin(), network.models.end(), [location](const auto& model) {
                return model.second.find_location(location->location_name) != nullptr;
            });
            if(!found)
                throw fail("no template has the location '" + location->location_name + "'");
            return forward_reachability_searcher::query_solution_t{tree};
        }

        // the expression reads variables of the network and the locations that it names, and is a boolean
        for(auto& identifier : graphedit::identifiers_of(std::get<expr::syntax_tree_t>(node_of(predicate))))
            if(!automata.symbols.contains(identifier) && !automata.external_symbols.contains(identifier) &&
               !locations.contains(identifier))
                throw fail("'" + identifier + "' is not a variable of the network (locations are named "
                           "<instance>@<location>, e.g. Main.lamp(1)@on)");
        auto query = locations.empty() ? forward_reachability_searcher::query_solution_t{tree}
                                       : forward_reachability_searcher::query_solution_t{tree, with_bound_locations(tree, locations)};
        try {
            query.is_solution(automata);
        } catch(const std::bad_variant_access&) {
            throw fail("the predicate is not a boolean expression");
        } catch(const std::exception& e) {
            throw fail(std::string{"the predicate cannot be evaluated: "} + e.what());
        }
        return query;
    }
}
