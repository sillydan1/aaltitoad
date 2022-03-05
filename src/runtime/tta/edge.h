#ifndef AALTITOAD_EDGE_H
#define AALTITOAD_EDGE_H
#include <aaltitoadpch.h>
#include <symbol_table.h>
#include "location.h"

struct edge_t {
    location_t from;
    location_t to;
    std::string guardExpression;
    std::string updateExpression;

    auto evaluate_updates(const symbol_table_t& environment) const -> symbol_table_t;
    auto is_satisfied(const symbol_table_t& environment) const -> bool;
};

#endif
