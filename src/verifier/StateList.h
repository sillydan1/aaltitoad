#ifndef AALTITOAD_STATELIST_H
#define AALTITOAD_STATELIST_H
#include <aaltitoadpch.h>
#include "runtime/tta.h"

struct SearchState {
    TTA tta;
    size_t prevStateHash;
    bool justTocked;
    bool operator==(const SearchState& other) const {
        return prevStateHash == other.prevStateHash
            && justTocked == other.justTocked
            && tta == other.tta;
    }
    bool operator!=(const SearchState& other) const {
        return !this->operator==(other);
    }
};

struct StateListBucket : std::vector<SearchState> {
    using cit = std::vector<SearchState>::const_iterator;
    bool operator==(const SearchState& s) const {
        return std::any_of(begin(), end(), [&s](cit it){ return *it == s; });
    }
    bool operator!=(const SearchState& s) const {
        return !this->operator==(s);
    }
};

class StateList {
public:
    using map_t = std::unordered_map<size_t, StateListBucket>;
    using iterator = map_t::iterator;
    using const_iterator = map_t::const_iterator;
    void AddSearchState(size_t hash, SearchState&& state) {
        m_map[hash].emplace_back(state);
    }
    bool contains(size_t hash) const {
        return m_map.find(hash) != m_map.end();
    }
    bool empty() const {
        return m_map.empty();
    }
    auto begin() {
        return m_map.begin();
    }
    auto end() {
        return m_map.end();
    }
    auto size() const {
        return m_map.size();
    }
    StateListBucket operator[](size_t hash) {
        return m_map[hash];
    }
private:
    map_t m_map;
};

#endif //AALTITOAD_STATELIST_H
