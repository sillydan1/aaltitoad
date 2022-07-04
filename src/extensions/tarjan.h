#ifndef AALTITOAD_TARJAN_H
#define AALTITOAD_TARJAN_H
#include "graph/yagraph.h"
#include <stack>

struct tarjan_decoration {
    unsigned int index;
    unsigned int low_link;
    bool on_stack;
};
template<typename T, typename E, typename K>
using scc_t = std::vector<ya::node_refference<T,E,K>>;
template<typename T, typename E, typename K>
using tarjan_decorations_t = std::unordered_map<K, tarjan_decoration>;
template<typename T, typename E, typename K>
using tarjan_stack = std::stack<ya::node_refference<T,E,K>>;

template<typename T, typename E, typename K>
void strong_connect(ya::node_refference<T,E,K> v,
                    const ya::graph<T, E, K>& input_graph,
                    tarjan_decorations_t<T,E,K>& decorations,
                    unsigned int& index,
                    tarjan_stack<T,E,K>& stack,
                    std::vector<scc_t<T,E,K>>& sccs) {
    decorations.insert(std::make_pair(v->first, tarjan_decoration{.index=index, .low_link=index, .on_stack=true}));
    auto& decoration_v = decorations[v->first];
    stack.push(v);
    index++;

    for(const auto& e : v->second.outgoing_edges) {
        auto& w = e->second.target;
        if(!decorations.contains(w->first)) {
            strong_connect(w, input_graph, decorations, index, stack, sccs);
            decoration_v.low_link = std::min(decoration_v.low_link, decorations.at(w->first).low_link);
        } else if(decorations.at(w->first).on_stack)
            decoration_v.low_link = std::min(decoration_v.low_link, decorations.at(w->first).index);
    }

    if(decoration_v.low_link == decoration_v.index) {
        scc_t<T,E,K> scc{};
        auto& w = stack.top();
        while(decorations.at(w->first).index >= decoration_v.index) {
            stack.pop();
            decorations.at(w->first).on_stack = false;
            scc.push_back(w);
            if(stack.empty())
                break;
            w = stack.top();
        }
        sccs.push_back(scc);
    }
}

template<typename T, typename E, typename K>
auto tarjan(ya::graph<T,E,K>& input_graph) {
    std::vector<scc_t<T,E,K>> sccs{};
    tarjan_decorations_t<T,E,K> search_decorations{};
    unsigned int index = 0;
    for(auto v = input_graph.nodes.begin(); v != input_graph.nodes.end(); ++v) {
        if(!search_decorations.contains(v->first)) {
            tarjan_stack<T,E,K> S{};
            strong_connect(v, input_graph, search_decorations, index, S, sccs);
        }
    }
    return sccs;
}

#endif //AALTITOAD_TARJAN_H
