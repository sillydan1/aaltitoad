#ifndef AALTITOAD_UPDATE_EXPRESSION_H
#define AALTITOAD_UPDATE_EXPRESSION_H
#include <aaltitoadpch.h>
#include "symbol_map.h"

struct update_expression_t {
    std::string target_identifier;
    std::string expression;
    /**
     * Evaluate the right-hand-side expression and return a differential
     * symbol map, containing the changed variable.
     * */
    symbol_map_t evaluate(const symbol_map_t& environment) const;
};

#endif
