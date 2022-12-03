#include "scoped_template_builder.h"
#include "scoped_interpreter.h"
#include <ntta/builder/ntta_builder.h>
#include <util/exceptions/parse_error.h>
#include <drivers/tree_interpreter.h>

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
                if(std::find(result.begin(), result.end(),i->str()) == result.end())
                    result.push_back(i->str());
                else
                    spdlog::error("duplicate template parameters: '{0}' in {1}", i->str(), instance.tta_template_name);
            }
        }
        return result;
    }

    auto scoped_template_builder::get_invocation_arguments(const model::tta_instance_t& instance, expr::interpreter& interpreter) -> std::vector<expr::symbol_value_t> {
        std::vector<expr::symbol_value_t> result{};
        std::smatch match;
        if (std::regex_search(instance.invocation.cbegin(), instance.invocation.cend(), match, param_section)) {
            auto m = match.str().substr(1, match.str().size() - 2); // remove the parentheses
            for(auto i = std::sregex_iterator(m.begin(), m.end(), arg_split); i != std::sregex_iterator(); ++i) {
                auto res = interpreter.parse(i->str());
                if(res != 0) {
                    spdlog::error("{0}: could not get arguments of tta invocation: {1}", instance.invocation, interpreter.error);
                    throw parse_error(interpreter.error);
                }
                result.push_back(interpreter.expression_result);
            }
        }
        return result;
    }

    void scoped_template_builder::instantiate_tta_recursively(const model::tta_instance_t& instance, const std::string& parent_name, ntta_builder& network_builder) { // NOLINT(misc-no-recursion)
        auto scoped_name = parent_name + "." + instance.invocation;
        try {
            // TODO: Gather errors and throw one aggregate exception
            if(!templates.contains(instance.tta_template_name))
                throw parse_error(instance.tta_template_name + ": no such template");
            auto& instance_template = templates.at(instance.tta_template_name);
            scoped_interpreter interpreter{{internal_symbols, external_symbols}};
            auto parameters = get_invocation_parameters(instance);
            auto arguments = get_invocation_arguments(instance, interpreter);
            if(arguments.size() != parameters.size()) {
                spdlog::error("{0}: provided arguments {1} does not match parameters {2}", instance.invocation, arguments.size(), parameters.size());
                throw parse_error(instance.invocation + ": provided arguments does not match parameters");
            }

            // Fill the argument table
            expr::symbol_table_t argument_table{};
            for(auto i = 0; i < parameters.size(); i++)
                argument_table[parameters[i]] = arguments[i];

            // TODO: parameterize declarations
            /*
             * MyInvocation(32, "hello, world!", 12.5f)
             *   - regex: (*,)
             * MyTemplate(a, b, c)
             *
             * declarations:
             *   - int someval.a := a
             *   - int someval.b := 1
             *   - string someval.c := b
             * expected outcome:
             *   - int someval.32 := 32
             *   - int someval.hello, world! := 1 // compiler error
             *   - string someval.12.5f := "hello, world!"
             * */

            // Construct the expression compiler
            if(interpreter.parse(instance_template.declarations) != 0)
                throw parse_error("parsing declarations: " + interpreter.error);
            internal_symbols *= interpreter.public_result;
            auto local_scope_declarations = interpreter.result;
            scoped_compiler c{local_scope_declarations, scoped_name + ".", {internal_symbols, external_symbols}};
            internal_symbols *= c.get_localized_symbols();

            // Construct the tta builder
            tta_builder builder{&c};
            builder.set_name(scoped_name);

            // Look for duplicate locations
            std::vector<std::string> locations{};
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

            // Add edges
            // TODO: parameterize guards & updates
            for(auto& edge : instance_template.edges) {
                std::optional<std::string> guard{};
                if(!trim_copy(edge.guard).empty())
                    guard = edge.guard;
                std::optional<std::string> update{};
                if(!trim_copy(edge.update).empty())
                    update = edge.update;
                builder.add_edge({.source=edge.source, .target=edge.target, .guard=guard, .update=update});
            }

            // Add the tta to the network
            network_builder.add_tta(builder);

            // Recursively add instances
            // TODO: use stl parallel constructs to compile faster
            // TODO: sequentially composed TTAs
            for(auto& template_instance : instance_template.instances)
                instantiate_tta_recursively(template_instance, scoped_name, network_builder);

        } catch (std::logic_error& e) {
            spdlog::error("error instantiating '{0}': {2}", scoped_name, e.what());
            throw e;
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
        ntta_builder builder{};
        instantiate_tta_recursively(t, "", builder);
        return builder.build_heap();
    }

    auto scoped_template_builder::find_instance_sccs() -> std::vector<scc_t<std::string,std::string,std::string>> {
        spdlog::trace("looking for infinite recursive structures");
        auto template_dependency_graph_builder = ya::graph_builder<std::string,std::string>{};
        for(auto& t : templates) {
            template_dependency_graph_builder.add_node({t.first});
            for(auto& i : t.second.instances)
                template_dependency_graph_builder.add_edge(t.first, i.tta_template_name, " instantiates ");
        }
        auto g = template_dependency_graph_builder.build();
        return tarjan(g);
    }

    void scoped_template_builder::throw_if_infinite_recursion_in_dependencies() {
        auto recursive_instantiations = find_instance_sccs();
        if(recursive_instantiations.empty()) {
            spdlog::trace("model doesn't have recursive instantiation");
            return;
        }
        spdlog::info("SCCs:");
        for(auto& scc : recursive_instantiations) {
            std::stringstream ss{};
            for(auto& s: scc)
                for(auto& e: s->second.outgoing_edges)
                    ss << "<"
                       << e->second.source->second.data
                       << e->first
                       << e->second.target->second.data
                       << ">\n";
            spdlog::info("\n[\n{0}]", ss.str());
        }
        // TODO: this might throw all the time (iirc, the tarjan algorithm includes trivial components)
        throw parse_error("cannot instantiate network due to infinitely recursive instantiation, "
                          "set verbosity to info or higher for detailed information");
    }
}
