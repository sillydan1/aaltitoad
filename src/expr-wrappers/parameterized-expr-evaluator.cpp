#include "parameterized-expr-evaluator.h"
#include "driver/evaluator.h"
#include "symbol_table.h"

namespace aaltitoad {
    parameterized_expr_evaluator::parameterized_expr_evaluator(const expr::symbol_table_ref_collection_t& environments, const expr::symbol_table_t& paramargs, const expr::symbol_operator& op) 
    : expr::evaluator{environments, op}, parameter_arguments{paramargs} {}

    parameterized_expr_evaluator::~parameterized_expr_evaluator() {

    }

    auto parameterized_expr_evaluator::find(const std::string& identifier) const -> expr::symbol_table_t::const_iterator {
        auto env_it = parameter_arguments.find(identifier);
        if(env_it != parameter_arguments.end())
            return env_it; 
        return expr::evaluator::find(identifier);
    }
}

