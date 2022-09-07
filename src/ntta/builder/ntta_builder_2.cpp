#include "ntta_builder_2.h"
#include "ntta_builder.h"

namespace aaltitoad {

    auto tta_builder2::set_instance_name(const std::string& name) -> tta_builder2& {
        instance_name = name;
        return *this;
    }

    auto tta_builder2::set_symbol_declarations(const std::string& decls) -> tta_builder2& {
        symbol_declarations = decls;
        return *this;
    }

    auto tta_builder2::add_sub_tta(const std::string& tta_name, const std::string& arguments) -> tta_builder2& {
        sub_tta_instances.push_back({tta_name, arguments});
        return *this;
    }

    auto tta_builder2::add_location(const std::string &name) -> tta_builder2 & {
        locations.insert(name);
        return *this;
    }

    auto tta_builder2::set_start_location(const std::string &name) -> tta_builder2 & {
        initial_location = name;
        return *this;
    }

    auto tta_builder2::set_main() -> tta_builder2 & {
        is_main = true;
        return *this;
    }

    auto tta_builder2::add_edge(const edge_builder2 &edge) -> tta_builder2 & {
        edges.push_back(edge);
        return *this;
    }

    auto tta_builder2::build(expr::symbol_table_t& symbols, expr::symbol_table_t& external_symbols) -> tta_builder {
        tta_builder b{symbols, external_symbols};
        for(auto& loc : locations)
            b.add_location(loc);
        b.set_starting_location(initial_location);
        for(auto& edge : edges)
            b.add_edge({edge.source, edge.target, edge.guard, edge.update});
        return b;
    }

    auto ntta_builder2::add_declarations(const std::string& decls) -> ntta_builder2& {
        symbol_declarations.push_back(decls);
        return *this;
    }

    auto ntta_builder2::add_external_declarations(const std::string& decls) -> ntta_builder2& {
        external_symbol_declarations.push_back(decls);
        return *this;
    }

    auto ntta_builder2::add_tta(const tta_builder2& builder) -> ntta_builder2& {
        tta_builders.insert({builder.instance_name, builder});
        return *this;
    }

    auto ntta_builder2::build() -> ntta_builder {
        expr::symbol_table_t s{}, e{};
        expr::interpreter i{s};
        for(auto& tta : tta_builders)
            s += i.interpret_declarations(tta.second.symbol_declarations);
        for(auto& d : symbol_declarations)
            s += i.interpret_declarations(d);
        for(auto& d : external_symbol_declarations)
            e += i.interpret_declarations(d);
        // TODO: Implement support for declarations that are initialized with other variables.
        //       One way of doing this could be:
        //       - remember which decl-strings fail
        //       - try once more to load previously failed decl-strings (throw if fails on this second try)
        //       Or we could do some dependency-graph detection (read up on static analysis tools)
        ntta_builder b{};
        b.add_symbols(s).add_external_symbols(e);
        // TODO: Check for dependency loops with tarjans SCC algorithm
        auto main_component = std::find_if(tta_builders.begin(), tta_builders.end(),
                                           [](const auto& t){ return t.second.is_main; });
        if(main_component == tta_builders.end())
            throw std::logic_error("no main tta");
        build_recursive(b, main_component->first, main_component, s, e);
        return b;
    }

    // TODO: Sequential composition
    void ntta_builder2::build_recursive(ntta_builder &builder, const std::string& name, const tta_builder2_it &it, expr::symbol_table_t& s, expr::symbol_table_t& e) {
        auto bb = it->second.build(s,e);
        builder.add_tta(name, bb);
        for(auto& subtta : it->second.sub_tta_instances) {
            auto itt = tta_builders.find(subtta.filename);
            if(itt == tta_builders.end())
                spdlog::error("no such tta: {}", subtta.filename);
            build_recursive(builder, subtta.parameterization, itt, s, e);
        }
    }
}
