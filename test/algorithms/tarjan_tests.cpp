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
    graph<std::string> my_graph{{
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
    graph<std::string> my_graph{{
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
    graph<std::string> my_graph{{
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
