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
#ifndef AALTITOAD_GRAPHEDIT_NETWORK_H
#define AALTITOAD_GRAPHEDIT_NETWORK_H
#include "ntta_model.h"
#include <ntta/tta.h>
#include <functional>
#include <map>
#include <memory>
#include <optional>

namespace aaltitoad::graphedit {
    // An instance of a template in a network: a tta of the network
    struct instance_t {
        // the scoped name of the instance, which is the name of its tta (e.g. "Main.lamp(1)")
        std::string name;
        // the key of the model of its template
        std::string model;
        // the scoped name of the instance that instantiated it, and the subcomponent (a vertex of the model of the
        // parent) that did; both are empty for the main template
        std::string parent;
        std::string subcomponent;
        // the private identifiers of the instance, as the template declares them (their symbols are named
        // "<name>.<identifier>")
        std::vector<std::string> local_identifiers;
        // the parameters of the template, with their arguments
        expr::symbol_table_t parameters;
    };

    // An edge of an instance, identified in the network by its identifier
    struct instance_edge_t {
        std::string instance;
        std::string model;
        edge_t edge;
    };

    // The network of the NTTA models of a project: the models, the problems that aaltitoad finds in them, and the
    // network that aaltitoad builds from them (unless problems prevent it)
    struct network_t {
        // the models, by key
        std::map<std::string, ntta_model_t> models{};
        // the problems of the models, by key (every model has an entry, possibly empty)
        std::map<std::string, std::vector<problem_t>> problems{};
        // the network, or nullptr if it cannot be built (see problems)
        std::unique_ptr<ntta_t> automata{};
        // the instances by their scoped names, and the edges of the network by their identifiers
        std::map<std::string, instance_t> instances{};
        std::map<std::string, instance_edge_t> edges{};
        // the names of the symbols that the models without vertices declare (the environment)
        std::set<std::string> external_symbols{};

        // Whether one of the problems is an error
        auto has_errors() const -> bool;
        // The instances of a model
        auto instances_of(const std::string& model) const -> std::vector<const instance_t*>;
    };

    // Analyze the NTTA models of a project and build their network
    auto build_network(std::map<std::string, ntta_model_t> models) -> network_t;

    // The result of parsing an expression (a guard, an update, declarations...) with the expression language of
    // aaltitoad
    struct parse_result_t {
        std::optional<expression_driver::language_result> result{};
        // why it could not be parsed (e.g. "syntax error, unexpected ':=' between column 3 and 5")
        std::string error{};
    };
    auto parse_expression(const std::string& text) -> parse_result_t;
    // Parse with a parser of aaltitoad's expression language (e.g. a scoped compiler), which reports syntax errors on
    // std::cerr: the result, or what went wrong
    auto parse_with(const std::function<expression_driver::language_result()>& parse) -> parse_result_t;

    // Declarations without the modifiers extern and persistent (which aaltitoad does not know), and whether there were
    // any. String literals are left as they are.
    auto strip_environment_modifiers(const std::string& declarations) -> std::pair<std::string, bool>;
    // The identifiers that an expression reads
    auto identifiers_of(const expr::syntax_tree_t& expression) -> std::set<std::string>;
}

#endif //AALTITOAD_GRAPHEDIT_NETWORK_H
