#ifndef AALTITOAD_SCOPED_TEMPLATE_BUILDER_H
#define AALTITOAD_SCOPED_TEMPLATE_BUILDER_H
#include "model.h"
#include <ntta/tta.h>
#include <extensions/tarjan.h>

namespace aaltitoad::huppaal {
    class scoped_template_builder {
        std::unordered_map<std::string, model::tta_template> templates{};
        std::vector<std::string> global_symbol_declarations{};
        expr::symbol_table_t internal_symbols{};
        expr::symbol_table_t external_symbols{};
    public:
        auto add_template(const model::tta_template& t) -> scoped_template_builder&;
        auto add_global_symbols(const std::string& d) -> scoped_template_builder&;
        auto build_heap() -> ntta_t*;
    private:
        auto instantiate_tta_recursively(const model::tta_instance_t& instance, const std::string& parent_name) -> std::vector<tta_t>;
        auto find_instance_sccs() -> std::vector<scc_t<std::string,std::string,std::string>>;
        void throw_if_infinite_recursion_in_dependencies();
    };
}

#endif //AALTITOAD_SCOPED_TEMPLATE_BUILDER_H
