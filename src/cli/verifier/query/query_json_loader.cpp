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
#include <ctl_syntax_tree.h>
#include <nlohmann/json.hpp>
#include <variant>
#include "expr-wrappers/ctl-interpreter.h"
#include "query_json_loader.h"
#include "symbol_table.h"
#include "util/warnings.h"

namespace aaltitoad {
    auto load_query_json_file(const std::string& json_file, std::initializer_list<std::reference_wrapper<expr::symbol_table_t>> environments) -> std::vector<ctl::syntax_tree_t> {
        try {
            std::vector<ctl::syntax_tree_t> result{};
            ctl_interpreter c{environments};
            std::ifstream f(json_file);
            auto data = nlohmann::json::parse(f);
            for(auto& q : data) {
                spdlog::trace("compiling query {0}", std::string{q["query"]});
                auto query = c.compile(q["query"]);
                std::stringstream ss{}; ss << query;
                spdlog::trace("resulting tree: {0}", ss.str());
                if(!aaltitoad::is_query_searchable(query))
                    warnings::warn(unsupported_query, std::string{q["query"]}+" is not supported by aaltitoad - ignoring");
                else
                    result.emplace_back(std::move(query));
            }
            return result;
        } catch (std::exception &e) {
            spdlog::error("unable to parse json file {0}: {1}", json_file, e.what());
            throw;
        }
    }

    auto is_query_searchable(const ctl::syntax_tree_t& q) -> bool {
        // Extend this when more search-algorithms are implemented
        return std::visit(ya::overload(
                    // If the CTL syntax tree is terminating in an expr-expression before any modals or quantifiers, then it is not searchable
                    [&](const expr::syntax_tree_t& v) -> bool { return false; },
                    // If the CTL syntax tree is terminating in a location before any modals or quantifiers, then it is not searchable
                    [&](const ctl::location_t &v) -> bool { return false; },
                    // If the CTL syntax tree is split into multiple queries seperated by some operator e.g.: 'EFxx && EFyy', then it is not supported
                    [&](const expr::operator_t& v) -> bool { return false; },
                    [&](const expr::root_t& v) -> bool { return is_query_searchable(q.children()[0]); },
                    [&](const ctl::modal_t &v) -> bool {
                        if(q.children().empty())
                            return false;

                        const auto& c = q.children()[0];
                        if(c.children().empty())
                            return false;

                        if(!std::holds_alternative<ctl::quantifier_t>(c.node))
                            return false;

                        const auto& cc = c.children()[0];
                        if(v.operator_type == ctl::modal_op_t::E && std::get<ctl::quantifier_t>(c.node).operator_type == ctl::quantifier_op_t::F)
                            return is_query_trivial(cc);

                        if(v.operator_type == ctl::modal_op_t::A && std::get<ctl::quantifier_t>(c.node).operator_type == ctl::quantifier_op_t::G) {
                            spdlog::debug("safety queries (A G <query>) are not supported yet. See issue #41 if you need this feature");
                            return false;
                        }

                        return false;
                    },
                    [&](const ctl::quantifier_t &v) -> bool { return false; },
                    [](auto&& v) -> bool {
                        auto s = std::string{"not a recognized CTL AST node"} + typeid(v).name();
                        throw std::logic_error(s.c_str());
                    }
        ), static_cast<const ctl::underlying_syntax_node_t&>(q.node));
    }

    auto is_query_trivial(const ctl::syntax_tree_t& q) -> bool {
        return std::visit(ya::overload(
                    [&](const expr::syntax_tree_t& v) -> bool { return true; },
                    [&](const expr::root_t& v) -> bool { return is_query_trivial(q.children()[0]); },
                    [&](const expr::operator_t& v) -> bool { return std::all_of(q.children().begin(), q.children().end(), is_query_trivial); },
                    [&](const ctl::location_t &v) -> bool { return true; },
                    [&](const ctl::modal_t &v) -> bool { return false; },
                    [&](const ctl::quantifier_t &v) -> bool { return false; },
                    [](auto&& v) -> bool {
                        auto s = std::string{"not a recognized CTL AST node"} + typeid(v).name();
                        throw std::logic_error(s.c_str());
                    }
                    ), static_cast<const ctl::underlying_syntax_node_t&>(q.node));
    }
}
