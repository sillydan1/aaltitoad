#include "scoped_template_builder.h"
#include <ntta/builder/ntta_builder.h>
#include <extensions/exceptions/parse_error.h>

namespace aaltitoad::huppaal {
    auto scoped_template_builder::add_template(const model::tta_template& t) -> scoped_template_builder& {
        templates[t.name] = t;
        return *this;
    }

    auto scoped_template_builder::add_global_symbols(const std::string& d) -> scoped_template_builder& {
        global_symbol_declarations.push_back(d);
        return *this;
    }

    auto scoped_template_builder::instantiate_tta_recursively(const model::tta_instance_t& instance) -> std::vector<tta_t> {
        std::vector<tta_t> result{};
        if(!templates.contains(instance.tta_template_name)) {
            spdlog::error("'{0}': no such template", instance.tta_template_name);
            return result;
        }

        tta_builder builder{internal_symbols, external_symbols};
        auto& t = templates.at(instance.tta_template_name);
        for(auto& location : t.locations)
            builder.add_location(location.id);
        builder.set_starting_location(t.initial_location.id);
        // TODO: parameterize declarations
        // TODO: compile declarations - add to current scope (internal only)

        for(auto& edge : t.edges) {
            // TODO: parameterize guards & updates
            // TODO: compile edges before adding them
            std::optional<std::string> guard{};
            if(!trim_copy(edge.guard).empty())
                guard = edge.guard;
            std::optional<std::string> update{};
            if(!trim_copy(edge.update).empty())
                update = edge.update;
            builder.add_edge({.source=edge.source, .target=edge.target,.guard=guard,.update=update});
        }

        result.push_back(builder.build());
        for(auto& i : t.instances) { // TODO: support sequentially composed TTAs
            auto instances = instantiate_tta_recursively(i);
            result.insert(result.end(), instances.begin(), instances.end());
        }
        return result;
    }

    auto scoped_template_builder::build_heap() -> ntta_t* {
        // Find the "Main" component and instantiate it
        // Instantiate from there
        auto main_it = std::find_if(templates.begin(), templates.end(),
                  [](const auto& t){ return t.second.is_main; });
        if(main_it == templates.end())
            throw parse_error("no main template");
        ntta_builder builder{};
        // TODO: add Global symbols (when the rest of aaltitoad support expr::tree_interpreter)
        // TODO: extend expr to expose the "access" of a declaration (i.e. public => global, anything else => local)

        // TODO: Check for infinite recursion (build dependency graph and look for SCCs) in instances
        // TODO: instantiate the main component too
        // TODO: use stl parallel constructs to compile faster
        for(auto& instance : main_it->second.instances) {
            auto tta_instances = instantiate_tta_recursively(instance);
            for(auto& tta : tta_instances)
                builder.add_tta(instance.id, tta);
        }
        return builder.build_heap();
    }

    auto scoped_template_builder::find_instance_sccs() -> std::vector<std::vector<std::string>> {
        
        return {};
    }
}
