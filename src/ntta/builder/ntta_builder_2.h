#ifndef AALTITOAD_NTTA_BUILDER_2_H
#define AALTITOAD_NTTA_BUILDER_2_H
#include <map>
#include <vector>
#include <string>
#include <set>
#include <ntta/tta.h>
#include "ntta_builder.h"

// TODO: This two-layer builder structure should be refactored...
namespace aaltitoad {
    // TODO: Rename to something else
    struct edge_builder2 {
        std::string source, target, guard{}, update{};
    };

    struct sub_tta {
        std::string filename, parameterization;
    };

    // TODO: Rename to something else
    struct tta_builder2 {
        auto set_instance_name(const std::string& name) -> tta_builder2&;
        auto set_symbol_declarations(const std::string& decls) -> tta_builder2&;
        auto add_sub_tta(const std::string& tta_name, const std::string& arguments) -> tta_builder2&;
        auto add_location(const std::string& name) -> tta_builder2&;
        auto set_start_location(const std::string& name) -> tta_builder2&;
        auto set_main() -> tta_builder2&;
        auto add_edge(const edge_builder2& edge) -> tta_builder2&;
        auto build(expr::symbol_table_t& symbols, expr::symbol_table_t& external_symbols) -> tta_builder;
        std::string instance_name{};
        std::string symbol_declarations{};
        std::string initial_location{};
        bool is_main = false;
        std::set<std::string> locations{};
        std::vector<edge_builder2> edges{};
        std::vector<sub_tta> sub_tta_instances{};
    };

    // TODO: Rename to something else
    struct ntta_builder2 {
        using tta_builder2_it = std::map<std::string, tta_builder2>::iterator;
        auto add_declarations(const std::string& decls) -> ntta_builder2&;
        auto add_external_declarations(const std::string& decls) -> ntta_builder2&;
        auto add_tta(const tta_builder2& builder) -> ntta_builder2&;
        auto build() -> ntta_builder;
        void build_recursive(ntta_builder& builder, const std::string& name, const tta_builder2_it& it, expr::symbol_table_t& s, expr::symbol_table_t& e);
        std::map<std::string, tta_builder2> tta_builders;
        std::vector<std::string> symbol_declarations;
        std::vector<std::string> external_symbol_declarations;
    };
}

#endif //AALTITOAD_NTTA_BUILDER_2_H
