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
#include <drivers/driver.h>
#include <nlohmann/json.hpp>
#include "query_json_loader.h"
#include "util/warnings.h"

namespace aaltitoad {
    auto load_query_json_file(const std::string& json_file, std::initializer_list<expr::symbol_table_ref_t> environments) -> std::vector<ctl::compiler::compiled_expr_t> {
        try {
            std::vector<ctl::compiler::compiled_expr_t> result{};
            ctl::compiler c{environments};
            std::ifstream f(json_file);
            auto data = nlohmann::json::parse(f);
            for(auto& q : data) {
                spdlog::trace("compiling query {0}", q["query"]);
                auto query = c.compile(q["query"]);
                if(!aaltitoad::is_query_searchable(query))
                    warnings::warn(unsupported_query, std::string(q["query"])+" is not supported by aaltitoad - ignoring");
                else
                    result.emplace_back(std::move(query));
            }
            return result;
        } catch (std::exception &e) {
            spdlog::error("unable to parse json file {0}: {1}", json_file, e.what());
            throw;
        }
    }

    auto is_query_searchable(const ctl::compiler::compiled_expr_t& q) -> bool {
        // Extend this when more search-algorithms are implemented
        return std::visit(ya::overload(
                [&](const expr::syntax_tree_t& v) -> bool { return false; },
                [&](const expr::operator_t& v) -> bool { return false; },
                [&](const ctl::location_t &v) -> bool { return false; },
                [&](const ctl::modal_op_t &v) -> bool {
                    if(q.children().empty())
                        return false;

                    const auto& c = q.children()[0];
                    if(c.children().empty())
                        return false;

                    const auto& cc = c.children()[0];
                    if(!std::holds_alternative<ctl::quantifier_t>(c.node))
                        return false;

                    if(v == ctl::modal_op_t::E && std::get<ctl::quantifier_t>(c.node) == ctl::quantifier_t::F)
                        return is_query_trivial(cc);

                    if(v == ctl::modal_op_t::A && std::get<ctl::quantifier_t>(c.node) == ctl::quantifier_t::G)
                        return is_query_trivial(cc);

                    return false;
                },
                [&](const ctl::quantifier_t &v) -> bool { return false; },
                [](auto&& v) {
                    auto s = std::string{"not a recognized CTL AST node"} + typeid(v).name();
                    throw std::logic_error(s.c_str());
                }
        ), static_cast<const ctl::underlying_syntax_node_t&>(q.node));
    }

    auto is_query_trivial(const ctl::compiler::compiled_expr_t& q) -> bool {
        return std::visit(ya::overload(
                [&](const expr::syntax_tree_t& v) -> bool { return true; },
                [&](const expr::operator_t& v) -> bool { return std::all_of(q.children().begin(), q.children().end(), is_query_trivial); },
                [&](const ctl::location_t &v) -> bool { return true; },
                [&](const ctl::modal_op_t &v) -> bool { return false; },
                [&](const ctl::quantifier_t &v) -> bool { return false; },
                [](auto&& v) {
                    auto s = std::string{"not a recognized CTL AST node"} + typeid(v).name();
                    throw std::logic_error(s.c_str());
                }
        ), static_cast<const ctl::underlying_syntax_node_t&>(q.node));
    }
}
