#ifndef AALTITOAD_SCOPED_TEMPLATE_BUILDER_H
#define AALTITOAD_SCOPED_TEMPLATE_BUILDER_H
#include "model.h"
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
        auto add_global_symbols(const std::string& d) -> scoped_template_builder&;
        auto build_heap() -> ntta_t*;
    private:
        void instantiate_tta_recursively(const model::tta_instance_t& instance,
                                         const std::string& parent_name,
                                         ntta_builder& network_builder);
        auto generate_dependency_graph() -> ya::graph<std::string,std::string,std::string>;
        auto find_instance_sccs(ya::graph<std::string,std::string,std::string>& g) -> std::vector<scc_t<std::string,std::string,std::string>>;
        void throw_if_infinite_recursion_in_dependencies();
        auto get_invocation_arguments(const model::tta_instance_t& instance, expr::interpreter& interpreter) -> std::vector<expr::symbol_value_t>;
        auto get_invocation_parameters(const model::tta_instance_t& instance) -> std::vector<std::string>;
    };
}

#endif //AALTITOAD_SCOPED_TEMPLATE_BUILDER_H
