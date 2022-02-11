#ifndef AALTITOAD_SYMBOL_MAP_H
#define AALTITOAD_SYMBOL_MAP_H
#include <aaltitoadpch.h>
#include "extensions/hash_combine"
#include "shunting-yard.h"
#include "extensions/cparse_extensions.h"

struct symbol_map_t : public TokenMap {
    /** Overwrite this map's elements with elements of other. */
    void operator+=(const symbol_map_t& other);
    auto operator==(const symbol_map_t& other) const;
    auto operator!=(const symbol_map_t& other) const;
};
/** Overwrite lhs' elements with elements of rhs. */
auto operator+(const symbol_map_t& lhs, const symbol_map_t& rhs) -> symbol_map_t;
/**
 * Calculate the differences between lhs and rhs, where
 * the resulting map are the differing symbols of rhs
 **/
auto operator-(const symbol_map_t& lhs, const symbol_map_t& rhs) -> symbol_map_t;

namespace std {
    template<>
    struct hash<symbol_map_t> {
        std::size_t operator()(const symbol_map_t& map) const {
            std::size_t state_hash = 0;
            for(auto& symbol : map.map()) {
                auto symbol_hash = std::hash<std::string>{}(symbol.first);
                switch(symbol.second->type) {
                    case INT:   hash_combine(symbol_hash, symbol.second.asInt()    * COMBINE_MAGIC_NUM); break;
                    case BOOL:  hash_combine(symbol_hash, symbol.second.asBool()   * COMBINE_MAGIC_NUM); break;
                    case REAL:  hash_combine(symbol_hash, symbol.second.asDouble() * COMBINE_MAGIC_NUM); break;
                    case STR:   hash_combine(symbol_hash, symbol.second.asString()); break;
                    case TIMER: hash_combine(symbol_hash, symbol.second.asDouble() * COMBINE_MAGIC_NUM); break;
                    default: spdlog::error("Symbol type '{0}' is not supported!", tokenTypeToString(symbol.second->type)); break;
                }
                hash_combine(state_hash, symbol_hash * COMBINE_MAGIC_NUM);
            }
            return state_hash;
        }
    };
}

#endif
