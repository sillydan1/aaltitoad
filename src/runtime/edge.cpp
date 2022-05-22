#include "edge.h"
#include "parser/interpreter.h"
#include "extensions/exceptions/ntta_error.h"

auto edge_t::evaluate_updates(const symbol_table_t& environment) const -> symbol_table_t {
    expr::interpreter drv{environment};
    auto res = drv.parse(updateExpression);
    if(res != 0)
        throw ntta_error::edge_update_error(*this, drv.error);
    return drv.result;
}

auto edge_t::is_satisfied(const symbol_table_t& environment) const -> bool {
    expr::interpreter drv{environment};
    auto res = drv.parse(guardExpression);
    if(res != 0)
        throw ntta_error::edge_guard_error(*this, drv.error);
    return std::get<bool>(drv.expression_result);
}
