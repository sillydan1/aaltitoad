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
#include <catch2/catch_test_macros.hpp>
#include "util/tarjan.h"

template<typename T, typename E, typename K>
bool compare_scc(const scc_t<T,E,K>& actual, const std::vector<T>& expected) {
    return std::all_of(expected.begin(),
                       expected.end(),
                       [&actual](const auto& s){
                           return std::find_if(actual.begin(), actual.end(), [&s](const auto& e){ return s == e->second.data; }) != actual.end();
                       });
}

TEST_CASE("givenGraphWithFourSCCs_whenLookingForSCCs_thenFourSCCsAreFound") {
    // Example graph:
    // https://upload.wikimedia.org/wikipedia/commons/6/60/Tarjan%27s_Algorithm_Animation.gif
    // The Strongly connected components should be:
    //  { L1, L4, L0 }
    //  { L3, L2 }
    //  { L6, L5 }
    //  { L7 }
    auto my_graph = ya::graph_builder<int,char>{}
                    .add_nodes({0,1,2,3,4,5,6,7})
                    .add_edges({{0,4,'a'},{4,1,'b'},{1,0,'c'}}) // SCC1
                    .add_edges({{5,6,'d'},{6,5,'e'}})           // SCC2
                    .add_edges({{2,3,'f'},{3,2,'g'}})           // SCC3
                    .add_edges({{7,7,'h'}})                     // SCC4
                    .add_edges({{7,6,'i'},{7,3,'j'},{6,2,'k'},{5,4,'l'},{5,1,'m'},{2,1,'n'}}) // Extras
                    .build();
    auto strongly_connected_components = tarjan(my_graph);
    REQUIRE(strongly_connected_components.size() == 4);
    std::vector<int> scc1 = {1, 4, 0};
    std::vector<int> scc2 = {3, 2};
    std::vector<int> scc3 = {6, 5};
    std::vector<int> scc4 = {7};
    bool found_scc1 = false, found_scc2 = false, found_scc3 = false, found_scc4 = false;
    for(auto& scc : strongly_connected_components) {
        found_scc1 |= compare_scc<int,char,int>(scc, scc1);
        found_scc2 |= compare_scc<int,char,int>(scc, scc2);
        found_scc3 |= compare_scc<int,char,int>(scc, scc3);
        found_scc4 |= compare_scc<int,char,int>(scc, scc4);
    }
    REQUIRE(found_scc1);
    REQUIRE(found_scc2);
    REQUIRE(found_scc3);
    REQUIRE(found_scc4);
}

TEST_CASE("givenNonLoopingGraph_whenSearchForSCCs_thenFourSCCsFound") {
    auto my_graph = ya::graph_builder<int,char>{}
                    .add_nodes({0,1,2,3})
                    .add_edges({{0,1,'a'},{0,2,'b'}})
                    .build();
    auto strongly_connected_components = tarjan(my_graph);
    REQUIRE(strongly_connected_components.size() == 4);
    std::vector<int> scc1 = {0};
    std::vector<int> scc2 = {1};
    std::vector<int> scc3 = {2};
    std::vector<int> scc4 = {3};
    bool found_scc1 = false;
    bool found_scc2 = false;
    bool found_scc3 = false;
    bool found_scc4 = false;
    for(auto& scc : strongly_connected_components) {
        found_scc1 |= compare_scc<int,char,int>(scc, scc1);
        found_scc2 |= compare_scc<int,char,int>(scc, scc2);
        found_scc3 |= compare_scc<int,char,int>(scc, scc3);
        found_scc4 |= compare_scc<int,char,int>(scc, scc4);
    }
    REQUIRE(found_scc1);
    REQUIRE(found_scc2);
    REQUIRE(found_scc3);
    REQUIRE(found_scc4);
}

TEST_CASE("givenGraphWithWithWeaklyConnectedComponents_whenSearchForSCCs_thenFourSCCsFound") {
    auto my_graph = ya::graph_builder<int,char>{}
                    .add_nodes({0,1,2,3})
                    .add_edges({{0,2,'a'},{1,2,'b'},{3,2,'c'},{2,2,'d'}})
                    .build();
    auto sccs = tarjan(my_graph);
    REQUIRE(sccs.size() == 4);
    std::vector<int> scc1 = {0};
    std::vector<int> scc2 = {1};
    std::vector<int> scc3 = {2};
    std::vector<int> scc4 = {3};
    bool found_scc1 = false, found_scc2 = false, found_scc3 = false, found_scc4 = false;
    for(auto& scc : sccs) {
        found_scc1 |= compare_scc<int,char,int>(scc, scc1);
        found_scc2 |= compare_scc<int,char,int>(scc, scc2);
        found_scc3 |= compare_scc<int,char,int>(scc, scc3);
        found_scc4 |= compare_scc<int,char,int>(scc, scc4);
    }
    REQUIRE(found_scc1);
    REQUIRE(found_scc2);
    REQUIRE(found_scc3);
    REQUIRE(found_scc4);
}

TEST_CASE("givenGraphWithWithOneLoopingNode_whenSearchForSCCs_thenOneSCCIsFound") {
    auto my_graph = ya::graph_builder<int,char>{}
                    .add_nodes({0})
                    .add_edges({{0,0,'a'}})
                    .build();
    auto sccs = tarjan(my_graph);
    REQUIRE(sccs.size() == 1);
    std::vector<int> scc1 = {0};
    bool found_scc1 = false;
    for(auto& scc : sccs)
        found_scc1 |= compare_scc<int,char,int>(scc, scc1);
    REQUIRE(found_scc1);
}

#include <graph>
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
/// This is not a "unit" test per se, it's more a usage example and api usability testing
TEST_CASE("givenComplicatedDatastructure_whenConstructingGraph_thenTheApiIsNotTooAnnoyingToUse") {
    auto g = ya::graph_builder<node_data,edge_data,std::string>{}
               .add_node({"0", {"zero"}})
               .add_node({"1", {"one"}})
               .add_edge("2","0",{"x := 3"})
               .add_nodes({{"3", {"three"}},{"4", {"four"}}})
               .add_edges({{"0","1",{"x := 2"}},{"1","0",{"x := 1"}}})
               .add_node({"2", {"two"}})
               .optimize() // optional
               .build();
    REQUIRE(g.nodes.size() == 5);
    REQUIRE(g.edges.size() == 3);
    for(auto& n : g.nodes) {
        std::cout << n.second.data.name << ":" << std::endl;
        for(auto& e : n.second.outgoing_edges)
            std::cout << "<" << n.second.data.name << ", '" << e->second.data.descriptor << "', " << e->second.target->second.data.name << ">" << std::endl;
    }
}
