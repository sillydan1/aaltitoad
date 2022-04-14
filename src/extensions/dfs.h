#ifndef AALTITOAD_DFS_H
#define AALTITOAD_DFS_H
#include "graph.h"
#include <optional>

struct dfs_decoration {
    bool visited = false;
    bool finished = false;
};

template<typename T>
auto has_cycle_dfs(const graph<T>& g, unsigned int node_index, std::unordered_map<unsigned int, dfs_decoration>& decorations) {
    if(decorations[node_index].finished)
        return false;
    if(decorations[node_index].visited)
        return true;
    decorations.at(node_index).visited = true;
    auto range = g.edges.equal_range(node_index);
    for(auto it = range.first; it != range.second; ++it) {
        if(has_cycle_dfs(g, it->second, decorations))
            return true;
    }
    decorations.at(node_index).finished = true;
    return false;
}

template<typename T>
auto has_cycle_dfs(const graph<T>& g) -> bool {
    std::unordered_map<unsigned int, dfs_decoration> decorations{};
    for(unsigned int i = 0; i < g.nodes.size(); i++) {
        auto b = has_cycle_dfs(g, i, decorations);
        if(b)
            return b;
    }
    return false;
}

#endif //AALTITOAD_DFS_H
