#ifndef AALTITOAD_DFS_H
#define AALTITOAD_DFS_H
#include "dep_graph.h"
#include <optional>

struct dfs_decoration {
    bool visited = false;
    bool finished = false;
};
template<typename T, typename E, typename K>
using dfs_decorations_t = std::unordered_map<K, dfs_decoration>;
template<typename T, typename E, typename K>
using dfs_allowlist_t = std::unordered_map<K, bool>;
template<typename T, typename E, typename K>
using allowance_func_t = std::function<bool(const ya::node_const_ref<T,E,K>&)>;

template<typename T, typename E, typename K>
auto has_cycle_dfs(const ya::node_const_ref<T,E,K>& n, dfs_decorations_t<T,E,K>& decorations, const allowance_func_t<T,E,K>& is_allowed) {
    if(!is_allowed(n))
        return false; // n is not in the allow_list, so don't include it in the dfs
    if(decorations[n->first].finished)
        return false;
    if(decorations[n->first].visited)
        return true;
    decorations.at(n->first).visited = true;

    for(auto& edge : n->second.outgoing_edges) {
        if(has_cycle_dfs<T,E,K>(edge->second.target, decorations, is_allowed))
            return true;
    }

    decorations.at(n->first).finished = true;
    return false;
}

template<typename T, typename E, typename K>
auto has_cycle_dfs(const ya::graph<T,E,K>& g) -> bool {
    dfs_decorations_t<T,E,K> decorations{};
    for(auto it = g.nodes.begin(); it != g.nodes.end(); ++it) {
        if(has_cycle_dfs<T,E,K>(it, decorations, [](auto&&){ return true; }))
            return true;
    }
    return false;
}

template<typename T, typename E, typename K>
auto has_cycle_dfs(const scc_t<T,E,K>& component) -> bool {
    dfs_decorations_t<T,E,K> decorations{};
    dfs_allowlist_t<T,E,K> allow_list{};
    for(auto& node : component)
        allow_list[node] = true;
    for(auto& node : component) {
        if(has_cycle_dfs<T,E,K>(node, decorations, [&allow_list](const auto& n){ return allow_list.contains(n); }))
            return true;
    }
    return false;
}

#endif //AALTITOAD_DFS_H
