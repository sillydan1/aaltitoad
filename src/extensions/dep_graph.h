#ifndef AALTITOAD_DEP_GRAPH_H
#define AALTITOAD_DEP_GRAPH_H
#include <vector>

template<typename N, typename E = void>
class node {
    template<typename T>
    struct edge_with_data_t {
        T data;
        node<N,T>* target;
    };
    struct edge_no_data_t {
        node<N>* target;
    };
public:
    using edge = std::conditional_t<std::is_void<E>::value, edge_no_data_t, edge_with_data_t<E>>;
    N data;
    std::vector<edge> outgoing_edges{};
};

template<typename T, typename E = void>
class dep_graph {
public:
    dep_graph(const std::initializer_list<T>& data_nodes) : nodes{} {
        nodes.reserve(data_nodes.size());
        for(auto& dn : data_nodes)
            nodes.push_back({dn});
    }
    dep_graph(std::vector<node<T>>&& nodes) : nodes{std::move(nodes)} {} // NOLINT(google-explicit-constructor)
    dep_graph(const std::vector<node<T>>& nodes) : nodes{nodes} {} // NOLINT(google-explicit-constructor)
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
        nodes[from_node_index].outgoing_edges.push_back({&nodes[to_node_index]});
    }
private:
    std::vector<node<T>> nodes;
};

template<typename node_t, typename edge_t>
struct data_node_t;
template<typename node_t, typename edge_t>
using data_node_list_t = std::vector<data_node_t<node_t,edge_t>>;
template<typename node_t, typename edge_t>
using data_node_list_it = typename data_node_list_t<node_t, edge_t>::iterator;
template<typename node_t, typename edge_t>
using data_node_list_cit = typename data_node_list_t<node_t, edge_t>::const_iterator;
template<typename T, typename node_t, typename edge_t>
using data_node_listit_map_t = std::unordered_map<data_node_list_it<node_t,edge_t>, T>;

template<typename node_t, typename edge_t>
struct data_node_t {
    struct edge {
        edge_t data;
        data_node_list_it<node_t,edge_t> target;
    };
    using edge_list_t = std::vector<edge>;
    node_t data;
    edge_list_t outgoing_edges;
};

template<typename node_t, typename edge_t>
struct data_graph {
    data_node_list_t<node_t, edge_t> nodes;
};


#endif
