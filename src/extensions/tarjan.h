#ifndef AALTITOAD_TARJAN_H
#define AALTITOAD_TARJAN_H
#include "graph.h"
#include <stack>

struct tarjan_element {
    unsigned int index;
    unsigned int low_link;
    bool on_stack;
};

template<typename T>
void strong_connect(const T& v, const graph<T>& input_graph, std::unordered_map<T,tarjan_element>& decorations, unsigned int& index, std::stack<T>& stack, std::vector<T>& scc) {
    decorations.insert(std::make_pair(v, tarjan_element{.index=index, .low_link=index, .on_stack=true}));
    auto& decoration_v = decorations[v];
    stack.push(v);
    index++;

    auto match_it = std::find(input_graph.nodes.begin(), input_graph.nodes.end(), v);
    unsigned int i = match_it - input_graph.nodes.begin();
    auto range = input_graph.edges.equal_range(i);
    for(auto w_it = range.first; w_it != range.second; ++w_it) {
        auto& w = input_graph.nodes[w_it->second];
        if(!decorations.contains(w))
            strong_connect(w, input_graph, decorations, index, stack, scc);
        else if(decorations.at(w).on_stack)
            decoration_v.low_link = std::min(decoration_v.low_link, decorations.at(w).index);
    }

    if(decoration_v.low_link == decoration_v.index) {
        auto w = stack.top();
        stack.pop();
        decorations.at(w).on_stack = false;
        scc.push_back(w);
        while(w != v) {
            w = stack.top();
            stack.pop();
            decorations.at(w).on_stack = false;
            scc.push_back(w);
        }
    }
}

template<typename T>
auto tarjan(const graph<T>& input_graph) {
    std::vector<std::vector<T>> sccs{};
    std::vector<T> scc{};
    std::stack<T> S{};
    std::unordered_map<T,tarjan_element> search_decorations{};
    unsigned int index = 0;
    for(auto& v : input_graph.nodes) {
        if(!search_decorations.contains(v)) {
            strong_connect(v, input_graph, search_decorations, index, S, scc);
            sccs.push_back(scc);
            scc = {};
        }
    }
    return sccs;
}

#endif //AALTITOAD_TARJAN_H
