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
using dfs_allowlist_t = std::unordered_map<const association_node_t<T>*, bool>;

template<typename T, typename F>
auto has_cycle_dfs(const association_node_t<T>* n, dfs_decorations_t<T>& decorations, F is_allowed) {
    if(!is_allowed(n))
        return false; // n is not in the allow_list, so don't include it in the dfs
    if(decorations[n].finished)
        return false;
    if(decorations[n].visited)
        return true;
    decorations.at(n).visited = true;

    for(auto& neighbor : n->outgoing_edges) {
        if(has_cycle_dfs<T>(neighbor, decorations, is_allowed))
            return true;
    }

    decorations.at(n).finished = true;
    return false;
}

template<typename T>
auto has_cycle_dfs(const association_graph<T>& g) -> bool {
    dfs_decorations_t<T> decorations{};
    for(auto& node : g.get_nodes()) {
        if(has_cycle_dfs<T>(&node, decorations, [](auto&&){ return true; }))
            return true;
    }
    return false;
}

template<typename T>
auto has_cycle_dfs(const scc_t<T>& component) -> bool {
    dfs_decorations_t<T> decorations{};
    dfs_allowlist_t<T> allow_list{};
    for(auto& node : component)
        allow_list[node] = true;
    for(auto& node : component) {
        if(has_cycle_dfs<T>(node, decorations, [&allow_list](const auto& n){ return allow_list.contains(n); }))
            return true;
    }
    return false;
}

#endif //AALTITOAD_DFS_H
