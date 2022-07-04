#include <catch2/catch_test_macros.hpp>
#include "extensions/graph_algorithms"

TEST_CASE("givenVerySimpleCyclicGraph_whenCheckingForCycles_thenReturnTrue") {
    auto my_graph = ya::graph_builder<std::string,std::string>{}
                    .add_node("L0", "L0")
                    .add_edge("L0", "L0", "L0->L0")
                    .build();
    REQUIRE(has_cycle_dfs<std::string,std::string,std::string>(my_graph));
}

TEST_CASE("givenAcyclicGraph_whenCheckingForCycles_thenReturnFalse") {
    auto my_graph = ya::graph_builder<int,char>{}
                    .add_nodes({0,1,2,3,4})
                    .add_edges({{0,1,'a'},{2,3,'b'},{0,4,'c'}})
                    .build();
    REQUIRE(!has_cycle_dfs(my_graph));
}

TEST_CASE("givenGraphWithACyclicSCC_whenCheckingSCCForCycles_thenReturnTrue") {
    auto my_graph = ya::graph_builder<int,char>{}
                    .add_nodes({0,1,2})
                    .add_edges({{0,1,'a'},{1,2,'b'},{2,0,'c'}})
                    .build();
    auto sccs = tarjan(my_graph);
    REQUIRE(sccs.size() == 1);
    for(auto& scc : sccs)
        REQUIRE(has_cycle_dfs<int,char,int>(scc));
}
/*
TEST_CASE("givenGraphWithAnAcyclicSCC_whenCheckingSCCForCycles_thenReturnFalse") {
    auto my_graph = ya::graph_builder<int,char>{}
                    .add_nodes({0,1,2})
                    .add_edges({{0,1,'a'},{1,2,'b'},{2,2,'c'}})
                    .build();
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
    auto my_graph = ya::graph_builder<int,char>{}
                    .add_nodes({0,1,2})
                    .add_edges({{0,1,'a'},{2,2,'b'}})
                    .build();
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
    auto my_graph = ya::graph_builder<int,char>{}
                    .add_nodes({0,1,2})
                    .add_edges({{0,1,'a'},{1,2,'b'}})
                    .build();
    auto sccs = tarjan(my_graph);
    REQUIRE(sccs.size() == 3);
    for(auto& scc : sccs)
        REQUIRE(!has_cycle_dfs<std::string>(scc));
}
*/