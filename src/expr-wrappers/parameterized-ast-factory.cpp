#include "parameterized-ast-factory.h"
#include "ast-factory.h"

namespace aaltitoad {
    parameterized_ast_factory::parameterized_ast_factory(const std::string& scope_prefix, const expr::symbol_table_t& paramargs, const std::vector<std::string>& local_names)
    : scope_prefix{scope_prefix}, parameter_arguments{paramargs}, local_identifiers{local_names} {}

    parameterized_ast_factory::~parameterized_ast_factory() {}

    auto parameterized_ast_factory::build_identifier(const std::string& identifier) -> expr::syntax_tree_t {
        // Is the identifier a parameter name?
        if(parameter_arguments.contains(identifier))
            return expr::ast_factory::build_literal(parameter_arguments.at(identifier));
        // Is the identifier a locally declared variable?
        if(std::find(local_identifiers.begin(), local_identifiers.end(), identifier) != local_identifiers.end())
            return expr::ast_factory::build_identifier(scope_prefix + identifier);
        // Otherwise, default case
        return expr::ast_factory::build_identifier(identifier);
    }
}

