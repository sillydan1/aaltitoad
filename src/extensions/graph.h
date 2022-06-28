#ifndef AALTITOAD_GRAPH_H
#define AALTITOAD_GRAPH_H
#include <vector>

template<typename T>
struct node {
    T data;
    std::vector<node*> outgoing_edges{};
};

template<typename T>
class graph {
public:
    graph(const std::initializer_list<T>& data_nodes) : nodes{} {
        nodes.reserve(data_nodes.size());
        for(auto& dn : data_nodes)
            nodes.push_back({dn});
    }
    graph(std::vector<node<T>>&& nodes) : nodes{std::move(nodes)} {} // NOLINT(google-explicit-constructor)
    graph(const std::vector<node<T>>& nodes) : nodes{nodes} {} // NOLINT(google-explicit-constructor)
    //// To keep edge pointers valid you must never add new nodes to
    //// an association_graph after you have constructed it.
    inline auto get_nodes() const -> const std::vector<node<T>>& {
        return nodes;
    }
    //// Insert a new edge from a node to another, by reference of
    //// the node indices. May throw std::logic_error if provided
    //// indices are not with range.
    void insert_edge(size_t from_node_index, size_t to_node_index) {
        if(from_node_index >= nodes.size())
            throw std::logic_error("Invalid start node index");
        if(to_node_index >= nodes.size())
            throw std::logic_error("Invalid target node index");
        nodes[from_node_index].outgoing_edges.push_back(&nodes[to_node_index]);
    }
private:
    std::vector<node<T>> nodes;
};

#endif
