#ifndef AALTITOAD_DFS_H
#define AALTITOAD_DFS_H
#include <graph>
#include <optional>

struct dfs_decoration {
    bool visited = false;
    bool finished = false;
};
template<typename T, typename E, typename K>
using dfs_decorations_t = std::unordered_map<ya::node_refference<T,E,K>, dfs_decoration, ya::node_ref_hasher<T,E,K>>;
template<typename T, typename E, typename K>
using dfs_allowlist_t = std::unordered_map<ya::node_refference<T,E,K>, bool, ya::node_ref_hasher<T,E,K>>;
template<typename T, typename E, typename K>
using allowance_func_t = std::function<bool(const ya::node_refference<T,E,K>&)>;

template<typename T, typename E, typename K>
auto has_cycle_dfs(ya::node_refference<T,E,K> n, dfs_decorations_t<T,E,K>& decorations, const allowance_func_t<T,E,K>& is_allowed) {
    if(!is_allowed(n))
        return false; // n is not in the allow_list, so don't include it in the dfs
    if(decorations[n].finished)
        return false;
    if(decorations[n].visited)
        return true;
    decorations.at(n).visited = true;

    for(auto& edge : n->second.outgoing_edges) {
        if(has_cycle_dfs<T,E,K>(edge->second.target, decorations, is_allowed))
            return true;
    }

    decorations.at(n).finished = true;
    return false;
}

template<typename T, typename E, typename K>
auto has_cycle_dfs(ya::graph<T,E,K>& g) -> bool {
    dfs_decorations_t<T,E,K> decorations{};
    for(auto it = g.nodes.begin(); it != g.nodes.end(); ++it) {
        if(has_cycle_dfs<T,E,K>(it, decorations, [](auto&&){ return true; }))
            return true;
    }
    return false;
}

template<typename T, typename E, typename K>
auto has_cycle_dfs(scc_t<T,E,K>& component) -> bool {
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
