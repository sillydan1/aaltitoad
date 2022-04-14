#ifndef AALTITOAD_GRAPH_H
#define AALTITOAD_GRAPH_H
#include <vector>

template<typename T>
struct graph {
    using vertex_collection = std::vector<T>;
    using vertex_ref = typename vertex_collection::const_iterator;

    vertex_collection nodes;
    std::unordered_multimap<unsigned int, unsigned int> edges; // key = from, value = to
};

#endif
