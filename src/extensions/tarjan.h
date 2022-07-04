#ifndef AALTITOAD_TARJAN_H
#define AALTITOAD_TARJAN_H
//#include "dep_graph.h"
#include "graph/yagraph.h"
#include <stack>

struct tarjan_decoration {
    unsigned int index;
    unsigned int low_link;
    bool on_stack;
};
template<typename T, typename E, typename K>
using scc_t = std::vector<const ya::node_refference<T,E,K>>;
template<typename T, typename E, typename K>
using tarjan_decorations_t = std::unordered_map<const ya::node_refference<T,E,K>, tarjan_decoration>;
template<typename T, typename E, typename K>
using tarjan_stack = std::stack<const ya::node_refference<T,E,K>>;

template<typename T, typename E, typename K>
void strong_connect(const ya::node_refference<T,E,K> v,
                    const ya::graph<T, E, K>& input_graph,
                    tarjan_decorations_t<T,E,K>& decorations,
                    unsigned int& index,
                    tarjan_stack<T,E,K>& stack,
                    std::vector<scc_t<T,E,K>>& sccs) {
    decorations.insert(std::make_pair(v, tarjan_decoration{.index=index, .low_link=index, .on_stack=true}));
    auto& decoration_v = decorations[v];
    stack.push(v);
    index++;

    for(const auto& e : v->outgoing_edges) {
        const auto* w = e.target;
        if(!decorations.contains(w)) {
            strong_connect(w, input_graph, decorations, index, stack, sccs);
            decoration_v.low_link = std::min(decoration_v.low_link, decorations.at(w).low_link);
        } else if(decorations.at(w).on_stack)
            decoration_v.low_link = std::min(decoration_v.low_link, decorations.at(w).index);
    }

    if(decoration_v.low_link == decoration_v.index) {
        scc_t<T,E,K> scc{};
        auto& w = stack.top();
        while(decorations.at(w).index >= decoration_v.index) {
            stack.pop();
            decorations.at(w).on_stack = false;
            scc.push_back(w);
            if(stack.empty())
                break;
            w = stack.top();
        }
        sccs.push_back(scc);
    }
}

//// Dev notes: T must be hashable and unique in the graph for this to work
template<typename T, typename E, typename K>
auto tarjan(const ya::graph<T,E,K>& input_graph) {
    std::vector<scc_t<T,E,K>> sccs{};
    tarjan_decorations_t<T,E,K> search_decorations{};
    unsigned int index = 0;
    for(auto& v : input_graph.get_nodes()) {
        if(!search_decorations.contains(&v)) {
            tarjan_stack<T,E,K> S{};
            strong_connect(&v, input_graph, search_decorations, index, S, sccs);
        }
    }
    return sccs;
}

#endif //AALTITOAD_TARJAN_H
