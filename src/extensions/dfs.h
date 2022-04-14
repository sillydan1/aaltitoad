#ifndef AALTITOAD_DFS_H
#define AALTITOAD_DFS_H
#include "graph.h"
#include <optional>

struct dfs_decoration {
    bool visited = false;
    bool finished = false;
};
template<typename T>
using dfs_decorations_t = std::unordered_map<const association_node_t<T>*, dfs_decoration>;

template<typename T>
auto has_cycle_dfs(const association_node_t<T>* n, dfs_decorations_t<T>& decorations) {
    if(decorations[n].finished)
        return false;
    if(decorations[n].visited)
        return true;
    decorations.at(n).visited = true;

    for(auto& neighbor : n->outgoing_edges) {
        if(has_cycle_dfs<T>(neighbor, decorations))
            return true;
    }

    decorations.at(n).finished = true;
    return false;
}

template<typename T>
auto has_cycle_dfs(const association_graph<T>& g) -> bool {
    dfs_decorations_t<T> decorations{};
    for(auto& node : g.get_nodes()) {
        if(has_cycle_dfs<T>(&node, decorations))
            return true;
    }
    return false;
}

template<typename T>
auto has_cycle_dfs(const scc_t<T>& component) -> bool {
    dfs_decorations_t<T> decorations{};
    for(auto& node : component) {
        if(has_cycle_dfs<T>(node, decorations))
            return true;
    }
    return false;
}

#endif //AALTITOAD_DFS_H
