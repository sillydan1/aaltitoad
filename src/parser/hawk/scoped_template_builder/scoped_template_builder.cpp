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

    auto scoped_template_builder::instantiate_tta_recursively(const model::tta_instance_t& instance,
                                                              const expr::symbol_table_tree_t::_left_df_iterator& root_scope,
                                                              const expr::symbol_table_tree_t::_left_df_iterator& parent_scope,
                                                              const std::string& parent_name) -> std::vector<tta_t> { // NOLINT(misc-no-recursion)
        try {
            std::vector<tta_t> result{};
            if(!templates.contains(instance.tta_template_name))
                throw parse_error(instance.tta_template_name + " no such template");

            auto& t = templates.at(instance.tta_template_name);

            // TODO: parameterize declarations

            scoped_interpreter interpreter{parent_scope};
            if(interpreter.parse(t.declarations) != 0)
                throw parse_error("parsing declarations: " + interpreter.error);
            root_scope->node *= interpreter.public_result; // Add the global result to the tree-root symbols (ignore re-definitions)
            auto my_scope = parent_scope->put(interpreter.result);

            tta_builder builder{internal_symbols, external_symbols}; // TODO: Port everything to use scoped stuff
            builder.set_name(parent_name + "." + instance.invocation);

            std::vector<std::string> locations{};
            std::vector<std::string> duplicate_locations{};
            for(auto& location: t.locations) {
                if(std::find(locations.begin(), locations.end(), location.id) != locations.end())
                    duplicate_locations.push_back(location.id);
                locations.push_back(location.id);
            }
            if(!duplicate_locations.empty())
                throw parse_error("duplicate locations: [" + join(",", duplicate_locations) + "]");
            builder.add_locations(locations);
            builder.set_starting_location(t.initial_location.id);

            for(auto& edge: t.edges) {
                // TODO: parameterize guards & updates
                std::optional<std::string> guard{};
                if(!trim_copy(edge.guard).empty())
                    guard = edge.guard;
                std::optional<std::string> update{};
                if(!trim_copy(edge.update).empty())
                    update = edge.update;
                builder.add_edge({.source=edge.source, .target=edge.target, .guard=guard, .update=update});
            }

            result.push_back(builder.build());
            for(auto& i: t.instances) { // TODO: sequentially composed TTAs
                auto instances = instantiate_tta_recursively(i,
                                                             root_scope,
                                                             my_scope,
                                                             parent_name + "." + instance.invocation);
                result.insert(result.end(), instances.begin(), instances.end());
            }
            return result;
        } catch (std::logic_error& e) {
            spdlog::error("error instantiating '{0}.{1}': {2}", parent_name, instance.invocation, e.what());
            throw e;
        }
    }

    auto scoped_template_builder::build_heap() -> ntta_t* {
        auto main_it = std::find_if(templates.begin(), templates.end(),[](const auto& t){ return t.second.is_main; });
        if(main_it == templates.end())
            throw parse_error("no main template");
        ntta_builder builder{};
        // TODO: add Global symbols (when the rest of aaltitoad support expr::tree_interpreter)
        // TODO: extend expr to expose the "access" of a declaration (i.e. public => global, anything else => local)

        throw_if_infinite_recursion_in_dependencies();

        // TODO: instantiate the main component too
        // TODO: use stl parallel constructs to compile faster
        spdlog::trace("building ntta from main component: '{0}'", main_it->second.name);
        for(auto& instance : main_it->second.instances) {
            auto tta_instances = instantiate_tta_recursively(instance,
                                                             main_it->second.name);
            for(auto& tta : tta_instances)
                builder.add_tta(instance.id, tta);
        }
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
        if(!recursive_instantiations.empty()) {
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
                              "set verbosity to info or higher for further information");
        }
        spdlog::trace("model doesn't have recursive instantiation");
    }
}
