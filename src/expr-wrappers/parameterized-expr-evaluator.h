#ifndef PARAMETERIZED_EXPR_EVALUATOR_H
#define PARAMETERIZED_EXPR_EVALUATOR_H
#include "driver/evaluator.h"
#include "interpreter.h"
#include "operations/symbol-operator.h"
#include "symbol_table.h"
#include <unordered_map>

namespace aaltitoad {
    class parameterized_expr_evaluator : public expr::evaluator {
    public:
        parameterized_expr_evaluator(const expr::symbol_table_ref_collection_t& environments, const expr::symbol_table_t& paramargs, const expr::symbol_operator& op); 
        ~parameterized_expr_evaluator() override;
        auto find(const std::string& identifier) const -> expr::symbol_table_t::const_iterator override;
    private:
        expr::symbol_table_t parameter_arguments;
        expr::symbol_table_ref_collection_t environments;
        expr::symbol_operator op;
    };
}

#endif // !PARAMETERIZED_EXPR_EVALUATOR_H
