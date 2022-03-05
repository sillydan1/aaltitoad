#include "edge.h"
#include "parser/driver.h"

auto edge_t::evaluate_updates(const symbol_table_t& environment) const -> symbol_table_t {
    driver drv{environment};
    auto res = drv.parse(updateExpression);
    if(res)
        throw std::logic_error("Unable to evaluate update expression "+updateExpression);
    return environment + drv.result;
}

auto edge_t::is_satisfied(const symbol_table_t& environment) const -> bool {
    // TODO: integrate expr:
    //       (https://github.com/sillydan1/expr)
    return false;
}
