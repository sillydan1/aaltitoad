#ifndef AALTITOAD_TARJAN_H
#define AALTITOAD_TARJAN_H
#include "graph.h"
#include <stack>

struct tarjan_decoration {
    unsigned int index;
    unsigned int low_link;
    bool on_stack;
};
template<typename T>
using scc_t = std::vector<const association_node_t<T>*>;
template<typename T>
using tarjan_decorations_t = std::unordered_map<const association_node_t<T>*, tarjan_decoration>;
template<typename T>
using tarjan_stack = std::stack<const association_node_t<T>*>;

template<typename T>
void strong_connect(const association_node_t<T>* v,
                    const association_graph<T>& input_graph,
                    tarjan_decorations_t<T>& decorations,
                    unsigned int& index,
                    tarjan_stack<T>& stack,
                    std::vector<scc_t<T>>& sccs) {
    decorations.insert(std::make_pair(v, tarjan_decoration{.index=index, .low_link=index, .on_stack=true}));
    auto& decoration_v = decorations[v];
    stack.push(v);
    index++;

    for(const auto& w : v->outgoing_edges) {
        if(!decorations.contains(w)) {
            strong_connect(w, input_graph, decorations, index, stack, sccs);
            decoration_v.low_link = std::min(decoration_v.low_link, decorations.at(w).low_link);
        } else if(decorations.at(w).on_stack)
            decoration_v.low_link = std::min(decoration_v.low_link, decorations.at(w).index);
    }

    if(decoration_v.low_link == decoration_v.index) {
        scc_t<T> scc{};
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
template<typename T>
auto tarjan(const association_graph<T>& input_graph) {
    std::vector<scc_t<T>> sccs{};
    tarjan_decorations_t<T> search_decorations{};
    unsigned int index = 0;
    for(auto& v : input_graph.get_nodes()) {
        if(!search_decorations.contains(&v)) {
            tarjan_stack<T> S{};
            strong_connect(&v, input_graph, search_decorations, index, S, sccs);
        }
    }
    return sccs;
}

#endif //AALTITOAD_TARJAN_H
