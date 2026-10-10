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
#include "scoped_template_builder.h"
#include "expr-wrappers/interpreter.h"
#include "scoped_interpreter.h"
#include "spdlog/spdlog.h"
#include "util/string_extensions.h"
#include "util/warnings.h"
#include <ntta/builder/ntta_builder.h>
#include <util/exceptions/parse_error.h>

namespace aaltitoad::hawk {
    auto scoped_template_builder::add_template(const model::tta_template& t) -> scoped_template_builder& {
        templates[t.name] = t;
        return *this;
    }

    auto scoped_template_builder::add_global_symbols(const std::string& d) -> scoped_template_builder& {
        global_symbol_declarations.push_back(d);
        return *this;
    }

    auto scoped_template_builder::get_invocation_parameters(const model::tta_instance_t& instance) -> std::vector<std::string> {
        std::vector<std::string> result{};
        std::smatch match;
        if (std::regex_search(instance.tta_template_name.cbegin(), instance.tta_template_name.cend(), match, param_section)) {
            auto m = match.str().substr(1, match.str().size() - 2); // remove the parentheses
            for(auto i = std::sregex_iterator(m.begin(), m.end(), arg_split); i != std::sregex_iterator(); ++i) {
                auto s = trim_copy(i->str());
                if(std::find(result.begin(), result.end(), s) != result.end())
                    throw parse_error(instance.tta_template_name + ": duplicate template parameters: " + s);
                result.push_back(s);
            }
        }
        return result;
    }

    auto scoped_template_builder::get_invocation_arguments(const model::tta_instance_t& instance, scoped_interpreter& interpreter) -> std::vector<expr::symbol_value_t> {
        std::vector<expr::symbol_value_t> result{};
        std::smatch match;
        if (std::regex_search(instance.invocation.cbegin(), instance.invocation.cend(), match, param_section)) {
            auto m = match.str().substr(1, match.str().size() - 2); // remove the parentheses
            for(auto i = std::sregex_iterator(m.begin(), m.end(), arg_split); i != std::sregex_iterator(); ++i)
                result.push_back(interpreter.parse_raw(trim_copy(i->str())));
        }
        return result;
    }

    auto scoped_template_builder::construct_interpreter_from_scope(const model::tta_instance_t& instance, const std::string& scoped_name) -> scoped_interpreter {
        // Interpret arguments and check for matching parameters
        scoped_interpreter interpreter{{internal_symbols, external_symbols}, scoped_name + "."};
        auto parameters = get_invocation_parameters(instance);
        auto arguments = get_invocation_arguments(instance, interpreter);
        if(arguments.size() != parameters.size()) {
            std::stringstream ss{}; ss << "provided arguments (" << arguments.size() << ") does not match parameters (" << parameters.size() << ")";
            throw parse_error(ss.str());
        }

        // Fill the parameter-argument table
        for(auto i = 0; i < parameters.size(); i++)
            interpreter.add_parameter(trim_copy(parameters[i]), arguments[i]);

        return interpreter;
    }

    void scoped_template_builder::parse_declarations_recursively(const model::tta_instance_t& instance, const std::string& parent_name) { // NOLINT(misc-no-recursion)
        auto scoped_name = (parent_name.empty() ? parent_name : parent_name + ".") + instance.invocation;
        spdlog::trace("{0}: parsing declarations", scoped_name);
        try {
            if(!templates.contains(instance.tta_template_name))
                throw parse_error(instance.tta_template_name + ": no such template");
            auto& instance_template = templates.at(instance.tta_template_name);

            // FIX: An instantiation cannot have a declaration that references a parent's declaration(s) - which _should_ be a feature of the hawk language, but is considered out of scope for aaltitoad v1.1.0
            auto interpreter = construct_interpreter_from_scope(instance, scoped_name);
            auto decls = interpreter.parse_declarations(instance_template.declarations);
            local_identifiers[scoped_name] = interpreter.get_local_identifiers();
            if(internal_symbols.is_overlapping(decls))
                warnings::warn(parser_warning, "double declaration detected");
            internal_symbols += decls;

            call_func_aggregate_errors(instance_template.instances, [this, &scoped_name](auto& template_instance){
               parse_declarations_recursively(template_instance, scoped_name);
            });

        } catch (std::exception& e) {
            spdlog::error("instantiating '{0}': {1}", scoped_name, e.what());
            throw;
        }
    }

    void scoped_template_builder::instantiate_tta_recursively(const model::tta_instance_t& instance, const std::string& parent_name, ntta_builder& network_builder) { // NOLINT(misc-no-recursion)
        auto scoped_name = (parent_name.empty() ? parent_name : parent_name + ".") + instance.invocation;
        spdlog::trace("{0}: instantiating", scoped_name);
        try {
            if(!templates.contains(instance.tta_template_name))
                throw parse_error(instance.tta_template_name + ": no such template");
            auto& instance_template = templates.at(instance.tta_template_name);

            call_func_aggregate_errors(instance_template.instances, [this, &scoped_name, &network_builder](auto& template_instance){
                instantiate_tta_recursively(template_instance, scoped_name, network_builder);
            });

            // NOTE: the private identifiers were found when the declarations of the instance were parsed
            auto interpreter = construct_interpreter_from_scope(instance, scoped_name);
            auto locals = local_identifiers.contains(scoped_name) ? local_identifiers.at(scoped_name) : std::vector<std::string>{};
            scoped_compiler c{locals, interpreter.get_parameters(), scoped_name + ".", {internal_symbols, external_symbols}};
            tta_builder builder{&c};
            builder.set_name(scoped_name);

            // Look for duplicate locations
            std::vector<std::string> locations{};
            locations.push_back(instance_template.initial_location.id);
            locations.push_back(instance_template.final_location.id);
            std::vector<std::string> duplicate_locations{};
            for(auto& location: instance_template.locations) {
                if(std::find(locations.begin(), locations.end(), location.id) != locations.end())
                    duplicate_locations.push_back(location.id);
                locations.push_back(location.id);
            }
            if(!duplicate_locations.empty())
                throw parse_error("duplicate locations: [" + join(",", duplicate_locations) + "]");

            // Add locations
            builder.add_locations(locations);
            builder.set_starting_location(instance_template.initial_location.id);

            // Add edges, identified by the instance and their ids (which must be unique in the template)
            scoped_instance_t built{.name=scoped_name, .instance=instance, .parent=parent_name, .local_identifiers=locals,
                                    .parameters=interpreter.get_parameters(), .edge_identifiers={}};
            std::unordered_map<std::string, int> edge_id_count{};
            for(auto& edge : instance_template.edges)
                edge_id_count[edge.id]++;
            call_func_aggregate_errors(instance_template.edges, [&builder, &built, &edge_id_count, &scoped_name](auto& edge){
                std::optional<std::string> guard{};
                if(!trim_copy(edge.guard).empty())
                    guard = edge.guard;
                std::optional<std::string> update{};
                if(!trim_copy(edge.update).empty())
                    update = edge.update;
                std::optional<std::string> identifier{};
                if(!edge.id.empty() && edge_id_count.at(edge.id) == 1) {
                    identifier = scoped_name + "." + edge.id;
                    built.edge_identifiers[edge.id] = identifier.value();
                }
                builder.add_edge({.source=edge.source, .target=edge.target, .guard=guard, .update=update, .identifier=identifier});
            });

            // Add the tta to the network
            network_builder.add_tta(builder);
            built_instances.push_back(std::move(built));

        } catch (std::exception& e) {
            spdlog::error("instantiating '{0}': {1}", scoped_name, e.what());
            throw;
        }
    }

    auto scoped_template_builder::build_heap() -> ntta_t* {
        auto main_it = std::find_if(templates.begin(), templates.end(),[](const auto& t){ return t.second.is_main; });
        if(main_it == templates.end())
            throw parse_error("no main template");
        throw_if_infinite_recursion_in_dependencies();
        model::tta_instance_t t{.id=main_it->first,
                                .tta_template_name=main_it->first,
                                .invocation=main_it->first};
        spdlog::trace("building ntta from main component: '{0}'", main_it->second.name);
        local_identifiers.clear();
        built_instances.clear();
        ntta_builder builder{};
        for(auto& decl : global_symbol_declarations)
            external_symbols += expression_driver{}.parse(decl).get_symbol_table();
        parse_declarations_recursively(t, "");
        instantiate_tta_recursively(t, "", builder);
        builder.add_symbols(internal_symbols);
        builder.add_external_symbols(external_symbols);
        return builder.build_heap();
    }

    auto scoped_template_builder::get_instances() const -> const std::vector<scoped_instance_t>& {
        return built_instances;
    }

    auto scoped_template_builder::generate_dependency_graph() -> ya::graph<std::string,std::string,std::string> {
        auto template_dependency_graph_builder = ya::graph_builder<std::string,std::string>{};
        for(auto& t : templates) {
            template_dependency_graph_builder.add_node({t.first});
            for(auto& i : t.second.instances)
                template_dependency_graph_builder.add_edge(t.first, i.tta_template_name, ya::uuid_v4());
        }
        spdlog::trace("building instantiation dependency graph");
        return template_dependency_graph_builder.validate().build();
    }

    auto scoped_template_builder::find_instance_sccs(ya::graph<std::string,std::string,std::string>& g) -> std::vector<scc_t<std::string,std::string,std::string>> {
        spdlog::trace("looking for infinite recursive structures");
        return tarjan(g);
    }

    void remove_trivial_sccs(std::vector<scc_t<std::string,std::string,std::string>>& sccs) {
        sccs.erase(std::remove_if(sccs.begin(), sccs.end(), [](auto& scc){
            return scc.size() <= 1;
        }), sccs.end());
    }

    void scoped_template_builder::throw_if_infinite_recursion_in_dependencies() {
        auto dependency_graph = generate_dependency_graph();
        auto recursive_instantiations = find_instance_sccs(dependency_graph);
        remove_trivial_sccs(recursive_instantiations);
        if(recursive_instantiations.empty()) {
            spdlog::trace("model doesn't have recursive instantiation");
            return;
        }
        spdlog::info("SCCs: {0}", recursive_instantiations.size());
        for(auto& scc : recursive_instantiations) {
            std::stringstream ss{};
            for(auto& s: scc) {
                for(auto& e: s->second.outgoing_edges)
                    ss << "\t"
                       << "[" << e->second.source->second.data << "](component:" << e->second.source->second.data << ")"
                       << " instantiates "
                       << "[" << e->second.target->second.data << "](component:" << e->second.target->second.data << ")"
                       << "\n";
            }
            spdlog::info("Loop: {0} [\n{1}]", scc.size(), ss.str());
        }
        throw parse_error("cannot instantiate network due to infinitely recursive instantiation, set verbosity to info or higher for detailed information");
    }

    auto scoped_template_builder::add_global_symbols(const std::vector<model::part_t>& parts) -> scoped_template_builder& {
        std::stringstream ss{};
        for(auto& p : parts)
            ss << p.id << " := " << p.value << ";";
        return add_global_symbols(ss.str());
    }
}
