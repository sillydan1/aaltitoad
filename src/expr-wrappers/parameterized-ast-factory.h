#ifndef PARAMETERIZED_AST_FACTORY_H
#define PARAMETERIZED_AST_FACTORY_H
#include "symbol_table.h"
#include <expr-lang/ast-factory.h>

namespace aaltitoad {
    class parameterized_ast_factory : public expr::ast_factory {
    public:
        parameterized_ast_factory(const std::string& scope_prefix, const expr::symbol_table_t& paramargs, const std::vector<std::string>& local_names);
        ~parameterized_ast_factory() override;
        // Builds identifier-nodes that are parameterized.
        // e.g.: parameters: foo :-> 32 and declaration 'private bar := 2' in template instance 'Main.Baz'
        // then expression 'bar + foo' would be converted to 'Main.Baz.bar + 32'
        auto build_identifier(const std::string& identifier) -> expr::syntax_tree_t override;
    private:
        std::string scope_prefix; // TODO: Should be string_views
        std::vector<std::string> local_identifiers;
        const expr::symbol_table_t& parameter_arguments;
    };
}

#endif

