#include <catch2/catch_test_macros.hpp>
#include "extensions/graph_algorithms"

TEST_CASE("givenVerySimpleCyclicGraph_whenCheckingForCycles_thenReturnTrue") {
    association_graph<std::string> my_graph{{{"L0"}}};
    my_graph.insert_edge(0,0);
    REQUIRE(has_cycle_dfs(my_graph));
}

TEST_CASE("givenAcyclicGraph_whenCheckingForCycles_thenReturnFalse") {
    association_graph<std::string> my_graph{{{"L0"}, {"L1"}, {"L2"}, {"L3"}, {"L4"}}};
    my_graph.insert_edge(0, 1);
    my_graph.insert_edge(2, 3);
    my_graph.insert_edge(0, 4);
    REQUIRE(!has_cycle_dfs(my_graph));
}

TEST_CASE("givenGraphWithACyclicSCC_whenCheckingSCCForCycles_thenReturnTrue") {
    association_graph<std::string> my_graph{{{"L0"}, {"L1"}, {"L2"}}};
    my_graph.insert_edge(0, 1);
    my_graph.insert_edge(1, 2);
    my_graph.insert_edge(2, 0);
    auto sccs = tarjan(my_graph);
    REQUIRE(sccs.size() == 1);
    for(auto& scc : sccs)
        REQUIRE(has_cycle_dfs<std::string>(scc));
}

TEST_CASE("givenGraphWithAnAcyclicSCC_whenCheckingSCCForCycles_thenReturnFalse") {
    association_graph<std::string> my_graph{{{"L0"}, {"L1"}, {"L2"}}};
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
    association_graph<std::string> my_graph{{{"L0"}, {"L1"}, {"L2"}}};
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
    association_graph<std::string> my_graph{{{"L0"}, {"L1"}, {"L2"}}};
    my_graph.insert_edge(0, 1);
    my_graph.insert_edge(1, 2);
    auto sccs = tarjan(my_graph);

    REQUIRE(sccs.size() == 3);
    for(auto& scc : sccs)
        REQUIRE(!has_cycle_dfs<std::string>(scc));
}

TEST_CASE("toyExampleOfCyclicAndNonCyclicSCCs", "[.]") {
    association_graph<std::string> my_graph{{
        {"L0"}, {"L1"}, {"L2"}, {"L3"},
        {"L4"}, {"L5"}, {"L6"}, {"L7"}
    }};
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
    //my_graph.insert_edge(7, 7); // Not cyclic.
    // Extras
    my_graph.insert_edge(7, 6);
    my_graph.insert_edge(7, 3);
    my_graph.insert_edge(6, 2);
    my_graph.insert_edge(5, 4);
    my_graph.insert_edge(5, 1);
    my_graph.insert_edge(2, 1);

    auto sccs = tarjan(my_graph);
    for(auto& scc : sccs) {
        std::cout << "{ ";
        for(auto& n : scc)
            std::cout << n->data << " ";
        std::cout << "} (cyclic: " << std::boolalpha << has_cycle_dfs<std::string>(scc) << ")" << std::endl;
    }
}
