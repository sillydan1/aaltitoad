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
    static diagnostic no_main_template{
        .identifier="no_main_template",
        .title="No main template",
        .message="Could not find a template marked as main",
        .description="A network of TTAs require a main entrypoint",
        .severity=Severity::SEVERITY_ERROR};
    diagnostic unknown_template(const std::string& template_name) {
        return {
        .identifier="unknown_template",
        .title="Template was not found",
        .message="Could not find a template with the name " + template_name,
        .description="All template references must be valid",
        .severity=Severity::SEVERITY_ERROR};
    }
    diagnostic duplicate_parameters(const std::string& parameters){
        return{
        .identifier="duplicate_parameters",
        .title="Multiple parameters of the same name detected: "+parameters,
        .message="Template parameter names must be unique",
        .description="Template parameter names must be unique in the context of the template",
        .severity=Severity::SEVERITY_ERROR};
    }
    static diagnostic duplicate_location{
        .identifier="duplicate_location",
        .title="Multiple locations with same identifier found",
        .message="Locations with same name is not allowed",
        .description="Locations must have a uniquely identifying id",
        .severity=Severity::SEVERITY_ERROR};
    diagnostic non_matching_args(const std::string& given, const std::string& expected) {
        return {
        .identifier="non_matching_args",
        .title="Wrong number of instantiation arguments",
        .message="Provided arguments ("+given+") does not match parameters ("+expected+")",
        .description="Arguments for instantiations should match the defined parameters",
        .severity=Severity::SEVERITY_ERROR};
    }
    diagnostic recursive_instantiation(const std::string& descriptor) {
        return {
        .identifier="recursive_instantiation",
        .title="Recursive instantiation detected",
        .message="There are loops in the instantiation tree: "+descriptor,
        .description="Instantiating templates should always be done in a directed acyclic graph",
        .severity=Severity::SEVERITY_ERROR};
    }

    static diagnostic duplicate_declaration{
        .identifier="duplicate_declaration",
        .title="Duplicate declarations detected",
        .message="Variable declarations are overwriting the parent's",
        .description="If a child declares the same variable identifiers as its parent's, the variable will be shadowed",
        .severity=Severity::SEVERITY_WARNING};
}

namespace aaltitoad::hawk {
    auto scoped_template_builder::add_template(const tta_template& t) -> scoped_template_builder& {
        templates[t.name] = t;
        return *this;
    }

    auto scoped_template_builder::add_global_symbols(const std::string& d) -> scoped_template_builder& {
        global_symbol_declarations.push_back(d);
        return *this;
    }

    auto scoped_template_builder::add_diagnostics(const std::vector<Diagnostic>& diags) -> scoped_template_builder& {
        diagnostics.insert(diagnostics.end(), diags.begin(), diags.end()); 
        return *this;
    }

    auto scoped_template_builder::get_invocation_parameters(const tta_instance_t& instance) -> std::optional<std::vector<std::string>> {
        std::vector<std::string> result{};
        std::smatch match;
        if (std::regex_search(instance.tta_template_name.cbegin(), instance.tta_template_name.cend(), match, param_section)) {
            auto m = match.str().substr(1, match.str().size() - 2); // remove the parentheses
            for(auto i = std::sregex_iterator(m.begin(), m.end(), arg_split); i != std::sregex_iterator(); ++i) {
                auto s = trim_copy(i->str());
                if(std::find(result.begin(), result.end(), s) != result.end()) {
                    diagnostics.push_back(diag_factory.with_model_key(instance.id).without_context().create_diagnostic(duplicate_parameters(s)));
                    return {};
                }
                result.push_back(s);
            }
        }
        return result;
    }

    auto scoped_template_builder::get_invocation_arguments(const tta_instance_t& instance, scoped_interpreter& interpreter) -> std::vector<expr::symbol_value_t> {
        std::vector<expr::symbol_value_t> result{};
        std::smatch match;
        if (std::regex_search(instance.invocation.cbegin(), instance.invocation.cend(), match, param_section)) {
            auto m = match.str().substr(1, match.str().size() - 2); // remove the parentheses
            for(auto i = std::sregex_iterator(m.begin(), m.end(), arg_split); i != std::sregex_iterator(); ++i)
                result.push_back(interpreter.parse_raw(trim_copy(i->str())));
        }
        return result;
    }

    auto scoped_template_builder::construct_interpreter_from_scope(const tta_instance_t& instance, const std::string& scoped_name) -> std::optional<scoped_interpreter> {
        // Interpret arguments and check for matching parameters
        scoped_interpreter interpreter{{internal_symbols, external_symbols}, scoped_name + "."};
        auto parameters = get_invocation_parameters(instance);
        if(!parameters.has_value())
            return {};
        auto arguments = get_invocation_arguments(instance, interpreter);
        if(arguments.size() != parameters.value().size()) {
            diagnostics.push_back(diag_factory.with_model_key(instance.id).with_context({}).create_diagnostic(non_matching_args(std::to_string(arguments.size()), std::to_string(parameters.value().size()))));
            return {};
        }

        // Fill the parameter-argument table
        for(auto i = 0; i < parameters.value().size(); i++)
            interpreter.add_parameter(trim_copy(parameters.value()[i]), arguments[i]);

        return interpreter;
    }

    void scoped_template_builder::parse_declarations_recursively(const tta_instance_t& instance, const std::string& parent_name) { // NOLINT(misc-no-recursion)
        auto scoped_name = (parent_name.empty() ? parent_name : parent_name + ".") + instance.invocation;
        spdlog::trace("{0}: parsing declarations", scoped_name);
        try {
            if(!templates.contains(instance.tta_template_name)) {
                diagnostics.push_back(diag_factory.with_model_key(parent_name).with_context({}).create_diagnostic(unknown_template(instance.tta_template_name)));
                return;
            }
            auto& instance_template = templates.at(instance.tta_template_name);

            // FIX: An instantiation cannot have a declaration that references a parent's declaration(s) - which _should_ be a feature of the hawk language, but is considered out of scope for aaltitoad v1.1.0
            auto interpreter = construct_interpreter_from_scope(instance, scoped_name);
            if(!interpreter.has_value())
                return;
            auto decls = interpreter.value().parse_declarations(instance_template.declarations);
            if(internal_symbols.is_overlapping(decls)) {
                diagnostics.push_back(diag_factory.with_model_key(instance.tta_template_name).with_context({}).create_diagnostic(duplicate_declaration));
                warnings::warn(parser_warning, "duplicate declaration detected");
            }
            internal_symbols += decls;

            for(auto& template_instance : instance_template.instances)
                parse_declarations_recursively(template_instance, scoped_name);
        } catch (std::exception& e) {
            spdlog::error("instantiating '{0}': {1}", scoped_name, e.what());
            throw;
        }
    }

    void scoped_template_builder::instantiate_tta_recursively(const tta_instance_t& instance, const std::string& parent_name, ntta_builder& network_builder) {
        auto scoped_name = (parent_name.empty() ? parent_name : parent_name + ".") + instance.invocation;
        spdlog::trace("{0}: instantiating", scoped_name);
        try {
            if(!templates.contains(instance.tta_template_name)) {
                diagnostics.push_back(diag_factory.with_model_key(parent_name).with_context(instance.id).create_diagnostic(unknown_template(instance.tta_template_name)));
                return;
            }
            auto& instance_template = templates.at(instance.tta_template_name);

            call_func_aggregate_errors(instance_template.instances, [this, &scoped_name, &network_builder](auto& template_instance){
                instantiate_tta_recursively(template_instance, scoped_name, network_builder);
            });

            auto interpreter = construct_interpreter_from_scope(instance, scoped_name);
            if(!interpreter.has_value())
                return;
            scoped_compiler c{interpreter.value().get_local_identifiers(), interpreter.value().get_parameters(), scoped_name + ".", {internal_symbols, external_symbols}};
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
            if(!duplicate_locations.empty()) {
                diagnostics.push_back(diag_factory.with_model_key(instance.tta_template_name).with_context(duplicate_locations).create_diagnostic(duplicate_location));
                return;
            }

            // Add locations
            builder.add_locations(locations);
            builder.set_starting_location(instance_template.initial_location.id);

            // Add edges
            call_func_aggregate_errors(instance_template.edges, [&builder](auto& edge){
                std::optional<std::string> guard{};
                if(!trim_copy(edge.guard).empty())
                    guard = edge.guard;
                std::optional<std::string> update{};
                if(!trim_copy(edge.update).empty())
                    update = edge.update;
                builder.add_edge({.source=edge.source, .target=edge.target, .guard=guard, .update=update});
            });

            // Add the tta to the network
            network_builder.add_tta(builder);

        } catch (std::exception& e) {
            spdlog::error("instantiating '{0}': {1}", scoped_name, e.what());
            throw;
        }
    }

    auto scoped_template_builder::error() const -> plugin::parse_error {
        return {
            .diagnostics=diagnostics,
        };
    }

    auto scoped_template_builder::build() -> plugin::parse_result {
        auto main_it = std::find_if(templates.begin(), templates.end(),[](const auto& t){ return t.second.is_main; });
        if(main_it == templates.end()) {
            diagnostics.push_back(diag_factory.without_context().create_diagnostic(no_main_template));
            return std::unexpected(error());
        }
        if(has_infinite_recursion_in_dependencies(main_it->first))
            return std::unexpected(error());
        tta_instance_t t{.id=main_it->first,
                                .tta_template_name=main_it->first,
                                .invocation=main_it->first};
        spdlog::trace("building ntta from main component: '{0}'", main_it->second.name);
        ntta_builder builder{};
        for(auto& decl : global_symbol_declarations)
            external_symbols += expression_driver{}.parse(decl).get_symbol_table();
        parse_declarations_recursively(t, "");
        instantiate_tta_recursively(t, "", builder); // TODO: prune duplicates
        builder.add_symbols(internal_symbols);
        builder.add_external_symbols(external_symbols);
        return plugin::parse_ok{
            .ntta=builder.build(),
            .diagnostics=diagnostics
        };
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

    auto scoped_template_builder::has_infinite_recursion_in_dependencies(const std::string& main_template) -> bool {
        auto dependency_graph = generate_dependency_graph();
        auto recursive_instantiations = find_instance_sccs(dependency_graph);
        remove_trivial_sccs(recursive_instantiations);
        if(recursive_instantiations.empty()) {
            spdlog::trace("model doesn't have recursive instantiation");
            return false;
        }
        spdlog::info("SCCs: {0}", recursive_instantiations.size());
        for(auto& scc : recursive_instantiations) {
            std::stringstream ss{};
            for(auto& s: scc) {
                for(auto& e: s->second.outgoing_edges)
                    ss << "\t"
                       << "[" << e->second.source->second.data << "]"
                       << " instantiates "
                       << "[" << e->second.target->second.data << "]"
                       << "\n";
            }
            diagnostics.push_back(diag_factory.with_model_key(main_template).without_context().create_diagnostic(recursive_instantiation(ss.str())));
        }
        return true;
    }

    auto scoped_template_builder::add_global_symbols(const std::vector<part_t>& parts) -> scoped_template_builder& {
        std::stringstream ss{};
        for(auto& p : parts)
            ss << p.id << " := " << p.value << ";";
        return add_global_symbols(ss.str());
    }
}
