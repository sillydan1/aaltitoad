#include <catch2/catch_test_macros.hpp>
#include "extensions/tarjan.h"

bool compare_scc(const std::vector<std::string>& actual, const std::vector<std::string>& expected) {
    return std::all_of(actual.begin(),
                       actual.end(),
                       [&expected](const auto& s){
                           return std::find(expected.begin(), expected.end(), s) != expected.end();
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
    graph<std::string> my_graph{};
    my_graph.nodes = {{"L0", "L1", "L2", "L3",
                          "L4", "L5", "L6", "L7"}};
    // SCC 1
    my_graph.edges.insert(std::make_pair(0, 4));
    my_graph.edges.insert(std::make_pair(4, 1));
    my_graph.edges.insert(std::make_pair(1, 0));
    // SCC 2
    my_graph.edges.insert(std::make_pair(5, 6));
    my_graph.edges.insert(std::make_pair(6, 5));
    // SCC 3
    my_graph.edges.insert(std::make_pair(2, 3));
    my_graph.edges.insert(std::make_pair(3, 2));
    // SCC 4
    my_graph.edges.insert(std::make_pair(7, 7));
    // Extras
    my_graph.edges.insert(std::make_pair(7, 6));
    my_graph.edges.insert(std::make_pair(7, 3));
    my_graph.edges.insert(std::make_pair(6, 2));
    my_graph.edges.insert(std::make_pair(5, 4));
    my_graph.edges.insert(std::make_pair(5, 1));
    my_graph.edges.insert(std::make_pair(2, 1));

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

TEST_CASE("givenNonLoopingGraph_whenSearchForSCCs_thenTwoSCCsFound") {
    graph<std::string> my_graph{};
    my_graph.nodes = {{"L0", "L1", "L2", "L3"}};
    my_graph.edges.insert(std::make_pair(0, 1));
    my_graph.edges.insert(std::make_pair(0, 2));
    auto strongly_connected_components = tarjan(my_graph);
    REQUIRE(strongly_connected_components.size() == 2);
    std::vector<std::string> scc1 = {"L0", "L1", "L2"};
    std::vector<std::string> scc2 = {"L3"};
    bool found_scc1 = false, found_scc2 = false;
    for(auto& scc : strongly_connected_components) {
        found_scc1 |= compare_scc(scc, scc1);
        found_scc2 |= compare_scc(scc, scc2);
    }
    REQUIRE(found_scc1);
    REQUIRE(found_scc2);
}
