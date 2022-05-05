#include "edge.h"
#include "parser/driver.h"
#include "extensions/exceptions/ntta_error.h"

auto edge_t::evaluate_updates(const symbol_table_t& environment) const -> symbol_table_t {
    driver drv{environment};
    auto res = drv.parse(updateExpression);
    if(res != 0)
        throw ntta_error::edge_update_error(*this, drv.error);
        // throw std::logic_error("edge: " + from + " -> " + to + " - update: '" + updateExpression + "' - " + std::get<std::string>(drv.error));
    return drv.result;
}

auto edge_t::is_satisfied(const symbol_table_t& environment) const -> bool {
    driver drv{environment};
    auto res = drv.parse(guardExpression);
    if(res != 0)
        throw ntta_error::edge_guard_error(*this, drv.error);
        //throw ntta_error("edge: " + from + " -> " + to + " - guard: '" + guardExpression + "' - " + std::get<std::string>(drv.error));
    return std::get<bool>(drv.expression_result);
}
