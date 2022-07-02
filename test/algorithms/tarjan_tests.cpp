#include <catch2/catch_test_macros.hpp>
#include "extensions/tarjan.h"

template<typename T>
bool compare_scc(const scc_t<T>& actual, const std::vector<T>& expected) {
    return std::all_of(expected.begin(),
                       expected.end(),
                       [&actual](const auto& s){
                           return std::find_if(actual.begin(), actual.end(), [&s](const auto& e){ return s == e->data; }) != actual.end();
                       });
}

TEST_CASE("givenGraphWithFourSCCs_whenLookingForSCCs_thenFourSCCsAreFound") {
    /**
     * Example graph:
     * https://upload.wikimedia.org/wikipedia/commons/6/60/Tarjan%27s_Algorithm_Animation.gif
     * The Strongly connected components should be:
     *  { L1, L4, L0 }
     *  { L3, L2 }
     *  { L6, L5 }
     *  { L7 }
     * */
    dep_graph<std::string> my_graph{{
        "L0", "L1", "L2", "L3",
        "L4", "L5", "L6", "L7"}};
    // SCC 1
    my_graph.insert_edge(0, 4);
    my_graph.insert_edge(4, 1);
    my_graph.insert_edge(1, 0);
    // SCC 2
    my_graph.insert_edge(5, 6);
    my_graph.insert_edge(6, 5);
    // SCC 3
    my_graph.insert_edge(2, 3);
    my_graph.insert_edge(3, 2);
    // SCC 4
    my_graph.insert_edge(7, 7);
    // Extras
    my_graph.insert_edge(7, 6);
    my_graph.insert_edge(7, 3);
    my_graph.insert_edge(6, 2);
    my_graph.insert_edge(5, 4);
    my_graph.insert_edge(5, 1);
    my_graph.insert_edge(2, 1);

    auto strongly_connected_components = tarjan(my_graph);

    REQUIRE(strongly_connected_components.size() == 4);
    std::vector<std::string> scc1 = {"L1", "L4", "L0"};
    std::vector<std::string> scc2 = {"L3", "L2"};
    std::vector<std::string> scc3 = {"L6", "L5"};
    std::vector<std::string> scc4 = {"L7"};
    bool found_scc1 = false, found_scc2 = false, found_scc3 = false, found_scc4 = false;
    for(auto& scc : strongly_connected_components) {
        found_scc1 |= compare_scc(scc, scc1);
        found_scc2 |= compare_scc(scc, scc2);
        found_scc3 |= compare_scc(scc, scc3);
        found_scc4 |= compare_scc(scc, scc4);
    }
    REQUIRE(found_scc1);
    REQUIRE(found_scc2);
    REQUIRE(found_scc3);
    REQUIRE(found_scc4);
}

TEST_CASE("givenNonLoopingGraph_whenSearchForSCCs_thenFourSCCsFound") {
    dep_graph<std::string> my_graph{{
        {"L0"},{"L1"},{"L2"},{"L3"}
    }};
    my_graph.insert_edge(0,1);
    my_graph.insert_edge(0,2);
    auto strongly_connected_components = tarjan(my_graph);
    REQUIRE(strongly_connected_components.size() == 4);
    std::vector<std::string> scc1 = {"L0"};
    std::vector<std::string> scc2 = {"L1"};
    std::vector<std::string> scc3 = {"L2"};
    std::vector<std::string> scc4 = {"L3"};
    bool found_scc1 = false;
    bool found_scc2 = false;
    bool found_scc3 = false;
    bool found_scc4 = false;
    for(auto& scc : strongly_connected_components) {
        found_scc1 |= compare_scc(scc, scc1);
        found_scc2 |= compare_scc(scc, scc2);
        found_scc3 |= compare_scc(scc, scc3);
        found_scc4 |= compare_scc(scc, scc4);
    }
    REQUIRE(found_scc1);
    REQUIRE(found_scc2);
    REQUIRE(found_scc3);
    REQUIRE(found_scc4);
}

TEST_CASE("givenGraphWithWithWeaklyConnectedComponents_whenSearchForSCCs_thenFourSCCsFound") {
    dep_graph<std::string> my_graph{{
        {"L0"}, {"L1"}, {"L2"}, {"L3"}}};
    my_graph.insert_edge(0, 2);
    my_graph.insert_edge(1, 2);
    my_graph.insert_edge(3, 2);
    my_graph.insert_edge(2, 2);
    auto sccs = tarjan(my_graph);
    REQUIRE(sccs.size() == 4);
    std::vector<std::string> scc1 = {"L0"};
    std::vector<std::string> scc2 = {"L1"};
    std::vector<std::string> scc3 = {"L2"};
    std::vector<std::string> scc4 = {"L3"};
    bool found_scc1 = false, found_scc2 = false, found_scc3 = false, found_scc4 = false;
    for(auto& scc : sccs) {
        found_scc1 |= compare_scc(scc, scc1);
        found_scc2 |= compare_scc(scc, scc2);
        found_scc3 |= compare_scc(scc, scc3);
        found_scc4 |= compare_scc(scc, scc4);
    }
    REQUIRE(found_scc1);
    REQUIRE(found_scc2);
    REQUIRE(found_scc3);
    REQUIRE(found_scc4);
}

#include "extensions/graph/yagraph.h"
struct node_data {
    std::string name;
};
struct edge_data {
    std::string descriptor;
};
namespace std {
    template<>
    struct hash<edge_data> {
        inline auto operator()(const edge_data& d) const -> size_t {
            return std::hash<std::string>{}(d.descriptor);
        }
    };
    template<>
    struct hash<node_data> {
        inline auto operator()(const node_data& d) const -> size_t {
            return std::hash<std::string>{}(d.name);
        }
    };
}
inline auto operator==(const edge_data& a, const edge_data& b) {
    return a.descriptor == b.descriptor;
}
inline auto operator==(const node_data& a, const node_data& b) {
    return a.name == b.name;
}
TEST_CASE("dwadwa") {
    ya::graph_builder<node_data,edge_data,std::string> builder{};
    auto g = builder
               .add_node("0", {"zero"})
               .add_node("1", {"one"})
               .add_edge("2","0",{"x := 3"})
               .add_nodes({{"3", {"three"}},{"4", {"four"}}})
               .add_edges({{"0","1",{"x := 2"}},{"1","0",{"x := 1"}}})
               .add_node("2", {"two"})
               .optimize() // optional
               .build();
    // REQUIRE(g.nodes.size() == xx);
    // REQUIRE(g.edges.size() == xx);
    for(auto& n : g.nodes) {
        std::cout << n.second.data.name << ":" << std::endl;
        for(auto& e : n.second.outgoing_edges)
            std::cout << "<" << n.second.data.name << ", '" << e->second.data.descriptor << "', " << e->second.target->second.data.name << ">" << std::endl;
    }
}
