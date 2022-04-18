#include "edge.h"
#include "parser/driver.h"

auto edge_t::evaluate_updates(const symbol_table_t& environment) const -> symbol_table_t {
    driver drv{environment};
    auto res = drv.parse(updateExpression);
    if(res)
        throw std::logic_error("Unable to evaluate update expression "+updateExpression);
    return drv.result;
}

auto edge_t::is_satisfied(const symbol_table_t& environment) const -> bool {
    driver drv{environment};
    auto res = drv.parse(guardExpression);
    if(res != 0)
        throw std::logic_error(std::get<std::string>(drv.error) + " in guard expression: " + guardExpression);
    return std::get<bool>(drv.expression_result);
}
