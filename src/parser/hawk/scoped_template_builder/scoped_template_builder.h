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
    // An instance of a template in a network that a scoped_template_builder built
    struct scoped_instance_t {
        // the scoped name of the instance, which is the name of its tta in the network (e.g. "Main.lamp(1)")
        std::string name;
        // what instantiated it (the main template is an instance of itself)
        model::tta_instance_t instance;
        // the scoped name of the instance that instantiated it (empty for the main template)
        std::string parent;
        // the private identifiers of the instance as the template declares them (their symbols are named
        // "<name>.<identifier>")
        std::vector<std::string> local_identifiers;
        // the parameters of the template, with their arguments
        expr::symbol_table_t parameters;
        // the ids of the edges of the template -> the identifiers of their edges in the network ("<name>.<id>"). Edges
        // without an id, or with the id of another edge of the template, get generated identifiers and are not listed
        std::unordered_map<std::string, std::string> edge_identifiers;
    };

    class scoped_template_builder {
        std::unordered_map<std::string, model::tta_template> templates{};
        std::vector<std::string> global_symbol_declarations{};
        expr::symbol_table_t internal_symbols{};
        expr::symbol_table_t external_symbols{};
        std::regex param_section{R"(\(.+(,.+)*\))"};
        std::regex arg_split{R"([\""].*[\""]|[^,]+)"};
        // scoped name -> the private identifiers of the instance
        std::unordered_map<std::string, std::vector<std::string>> local_identifiers{};
        std::vector<scoped_instance_t> built_instances{};
    public:
        auto add_template(const model::tta_template& t) -> scoped_template_builder&;
        auto add_global_symbols(const std::vector<model::part_t>& parts) -> scoped_template_builder&;
        auto add_global_symbols(const std::string& d) -> scoped_template_builder&;
        auto build_heap() -> ntta_t*;
        // The instances of the network that build_heap built (in no particular order)
        auto get_instances() const -> const std::vector<scoped_instance_t>&;
    private:
        auto construct_interpreter_from_scope(const model::tta_instance_t& instance, const std::string& scoped_name) -> scoped_interpreter;
        void parse_declarations_recursively(const model::tta_instance_t& instance, const std::string& parent_name);
        void instantiate_tta_recursively(const model::tta_instance_t& instance, const std::string& parent_name, ntta_builder& network_builder);
        auto generate_dependency_graph() -> ya::graph<std::string,std::string,std::string>;
        auto find_instance_sccs(ya::graph<std::string,std::string,std::string>& g) -> std::vector<scc_t<std::string,std::string,std::string>>;
        void throw_if_infinite_recursion_in_dependencies();
        auto get_invocation_arguments(const model::tta_instance_t& instance, scoped_interpreter& interpreter) -> std::vector<expr::symbol_value_t>;
        auto get_invocation_parameters(const model::tta_instance_t& instance) -> std::vector<std::string>;
    };
}

#endif //AALTITOAD_SCOPED_TEMPLATE_BUILDER_H
