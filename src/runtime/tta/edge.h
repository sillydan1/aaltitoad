#ifndef AALTITOAD_EDGE_H
#define AALTITOAD_EDGE_H
#include <aaltitoadpch.h>
#include "symbol_map.h"
#include "update_expression.h"
#include "location.h"

struct edge_t {
    location_t from;
    location_t to;
    std::string guardExpression;
    std::vector<update_expression_t> updateExpressions;
    symbol_map_t evaluate(const symbol_map_t& environment) const;
};

#endif
