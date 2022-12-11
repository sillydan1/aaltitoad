/**
 * aaltitoad - a verification engine for tick tock automata models
   Copyright (C) 2023 Asger Gitz-Johansen

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef AALTITOAD_TARJAN_H
#define AALTITOAD_TARJAN_H
#include <graph>
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
    auto& decoration_v = decorations.at(v->first);
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

    if(decoration_v.low_link != decoration_v.index)
        return;

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

template<typename T, typename E, typename K>
auto tarjan(ya::graph<T,E,K>& input_graph) {
    std::vector<scc_t<T,E,K>> sccs{};
    tarjan_decorations_t<T,E,K> search_decorations{};
    unsigned int index = 0;
    for(auto v = input_graph.nodes.begin(); v != input_graph.nodes.end(); ++v) {
        if(!search_decorations.contains(v->first)) {
            tarjan_stack<T,E,K> S{};
            strong_connect<T,E,K>(v, input_graph, search_decorations, index, S, sccs);
        }
    }
    return sccs;
}

#endif //AALTITOAD_TARJAN_H
