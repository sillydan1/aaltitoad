#ifndef AALTITOAD_EDGE_H
#define AALTITOAD_EDGE_H
#include <aaltitoadpch.h>
#include <symbol_table.h>
#include "location.h"

struct edge_t {
    using location_it = location_map_t::const_iterator;
    location_it from;
    location_it to;
    std::string guardExpression;
    std::string updateExpression;

    explicit edge_t(const location_it& from, const location_it& to, std::string guardExpression, std::string updateExpression) :
        from{from}, to{to}, guardExpression{std::move(guardExpression)}, updateExpression{std::move(updateExpression)} {}

    auto evaluate_updates(const symbol_table_t& environment) const -> symbol_table_t;
    auto is_satisfied(const symbol_table_t& environment) const -> bool;
};

using edge_list_t = std::vector<edge_t>;

#endif
