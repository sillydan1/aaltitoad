#include <catch2/catch_test_macros.hpp>
#include "extensions/graph_algorithms"

TEST_CASE("givenVerySimpleCyclicGraph_whenCheckingForCycles_thenReturnTrue") {
    graph<std::string> my_graph{};
    my_graph.nodes = {{"L0"}};
    my_graph.edges.insert(std::make_pair(0, 0));
    REQUIRE(has_cycle_dfs(my_graph));
}

TEST_CASE("givenAcyclicGraph_whenCheckingForCycles_thenReturnFalse") {
    graph<std::string> my_graph{};
    my_graph.nodes = {{"L0","L1", "L2", "L3", "L4"}};
    my_graph.edges.insert(std::make_pair(0, 1));
    my_graph.edges.insert(std::make_pair(2, 3));
    my_graph.edges.insert(std::make_pair(0, 4));
    REQUIRE(!has_cycle_dfs(my_graph));
}

TEST_CASE("experimental") {
    association_graph<std::string> my_graph{{{"L0"}, {"L1"}, {"L2"}}};
    my_graph.insert_edge(0,1);
    my_graph.insert_edge(1,2);
    my_graph.insert_edge(2,0);
}
