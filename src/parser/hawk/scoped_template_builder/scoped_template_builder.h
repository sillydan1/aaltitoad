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
#ifndef AALTITOAD_SCOPED_TEMPLATE_BUILDER_H
#define AALTITOAD_SCOPED_TEMPLATE_BUILDER_H
#include "model.h"
#include "scoped_interpreter.h"
#include <ntta/tta.h>
#include <util/tarjan.h>
#include <ntta/builder/ntta_builder.h>

namespace aaltitoad::hawk {
    class scoped_template_builder {
        std::unordered_map<std::string, model::tta_template> templates{};
        std::vector<std::string> global_symbol_declarations{};
        expr::symbol_table_t internal_symbols{};
        expr::symbol_table_t external_symbols{};
        std::regex param_section{R"(\(.+(,.+)*\))"};
        std::regex arg_split{R"([\""].*[\""]|[^,]+)"};
    public:
        auto add_template(const model::tta_template& t) -> scoped_template_builder&;
        auto add_global_symbols(const std::vector<model::part_t>& parts) -> scoped_template_builder&;
        auto add_global_symbols(const std::string& d) -> scoped_template_builder&;
        auto build_heap() -> ntta_t*;
    private:
        auto construct_interpreter_from_scope(const model::tta_instance_t& instance, const std::string& scoped_name) -> scoped_interpreter;
        void parse_declarations_recursively(const model::tta_instance_t& instance, const std::string& parent_name);
        void instantiate_tta_recursively(const model::tta_instance_t& instance, const std::string& parent_name, ntta_builder& network_builder);
        auto generate_dependency_graph() -> ya::graph<std::string,std::string,std::string>;
        auto find_instance_sccs(ya::graph<std::string,std::string,std::string>& g) -> std::vector<scc_t<std::string,std::string,std::string>>;
        void throw_if_infinite_recursion_in_dependencies();
        auto get_invocation_arguments(const model::tta_instance_t& instance, expr::interpreter& interpreter) -> std::vector<expr::symbol_value_t>;
        auto get_invocation_parameters(const model::tta_instance_t& instance) -> std::vector<std::string>;
    };
}

#endif //AALTITOAD_SCOPED_TEMPLATE_BUILDER_H
