#ifndef AALTITOAD_GRAPH_H
#define AALTITOAD_GRAPH_H
#include <vector>

template<typename T>
struct graph {
    using vertex_collection = std::vector<T>;
    using edge_map = std::unordered_multimap<unsigned int, unsigned int>;

    vertex_collection nodes;
    edge_map edges; // key = from, value = to
};

#endif
