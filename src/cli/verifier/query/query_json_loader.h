#ifndef AALTITOAD_QUERY_JSON_LOADER_H
#define AALTITOAD_QUERY_JSON_LOADER_H
#include <ctl_compiler.h>

namespace aaltitoad {
    auto load_query_json_file(const std::string& json_file, std::initializer_list<expr::symbol_table_ref_t> environments) -> std::vector<ctl::compiler::compiled_expr_t>;
}

#endif //AALTITOAD_QUERY_JSON_LOADER_H
