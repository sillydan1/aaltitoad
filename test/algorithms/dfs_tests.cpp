#include <catch2/catch_test_macros.hpp>
#include "extensions/graph_algorithms"

TEST_CASE("givenVerySimpleCyclicGraph_whenCheckingForCycles_thenReturnTrue") {
    auto my_graph = ya::graph_builder<std::string,std::string>{}.build();
    // dep_graph<std::string> my_graph{{"L0"}};
    // my_graph.insert_edge(0,0);
    REQUIRE(has_cycle_dfs<std::string,std::string,std::string>(my_graph));
}
/*
TEST_CASE("givenAcyclicGraph_whenCheckingForCycles_thenReturnFalse") {
    dep_graph<std::string> my_graph{{"L0", "L1", "L2", "L3", "L4"}};
    my_graph.insert_edge(0, 1);
    my_graph.insert_edge(2, 3);
    my_graph.insert_edge(0, 4);
    REQUIRE(!has_cycle_dfs(my_graph));
}

TEST_CASE("givenGraphWithACyclicSCC_whenCheckingSCCForCycles_thenReturnTrue") {
    dep_graph<std::string> my_graph{{"L0", "L1", "L2"}};
    my_graph.insert_edge(0, 1);
    my_graph.insert_edge(1, 2);
    my_graph.insert_edge(2, 0);
    auto sccs = tarjan(my_graph);
    REQUIRE(sccs.size() == 1);
    for(auto& scc : sccs)
        REQUIRE(has_cycle_dfs<std::string>(scc));
}

TEST_CASE("givenGraphWithAnAcyclicSCC_whenCheckingSCCForCycles_thenReturnFalse") {
    dep_graph<std::string> my_graph{{"L0", "L1", "L2"}};
    my_graph.insert_edge(0, 1);
    my_graph.insert_edge(1, 2);
    my_graph.insert_edge(2, 2);
    auto sccs = tarjan(my_graph);

    REQUIRE(sccs.size() == 3);
    for(auto& scc : sccs) {
        if(std::find_if(scc.begin(), scc.end(), [](const auto& n){ return n->data == "L2"; }) != scc.end())
            REQUIRE(has_cycle_dfs<std::string>(scc));
        else
            REQUIRE(!has_cycle_dfs<std::string>(scc));
    }
}

TEST_CASE("givenGraphWithDisconnectedNode_whenCheckingSCCForCycles_thenSomeCyclesAreFound") {
    dep_graph<std::string> my_graph{{"L0", "L1", "L2"}};
    my_graph.insert_edge(0, 1);
    my_graph.insert_edge(2, 2);
    auto sccs = tarjan(my_graph);

    REQUIRE(sccs.size() == 3);
    for(auto& scc : sccs) {
        if(std::find_if(scc.begin(), scc.end(), [](const auto& n){ return n->data == "L2"; }) != scc.end())
            REQUIRE(has_cycle_dfs<std::string>(scc));
        else
            REQUIRE(!has_cycle_dfs<std::string>(scc));
    }
}

TEST_CASE("givenGraphWithCycleAndNonCycle_whenCheckingSCCForCycles_thenSomeCyclesAreFound") {
    dep_graph<std::string> my_graph{{"L0", "L1", "L2"}};
    my_graph.insert_edge(0, 1);
    my_graph.insert_edge(1, 2);
    auto sccs = tarjan(my_graph);

    REQUIRE(sccs.size() == 3);
    for(auto& scc : sccs)
        REQUIRE(!has_cycle_dfs<std::string>(scc));
}
*/