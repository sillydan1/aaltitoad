#include <runtime/tta/ntta.h>
#include <catch2/catch_test_macros.hpp>

//// Test Tick Tock Automata - Contains only one component called "Main":
//// <Main>:
////    L1 -g1-u1-> L2
//// where:
////    g1: a >= 0
////    u1: b := true
//// with:
////    symbols = [a :-> 0, b :-> false]
////    initial_location = L1
////
ntta_t generate_test_main_tta() {
    symbol_table_t symbols{};
    symbols["a"] = 0;
    symbols["b"] = false;
    location_map_t locations{};
    locations.insert({"L1", location_t{}});
    locations.insert({"L2", location_t{}});
    edge_list_t edges{};
    auto g1 = "a >= 0";
    auto u1 = "b := true";
    edges.emplace_back(locations.find("L1"), locations.find("L2"), g1, u1);
    component_map_t components{};
    components.insert({"Main", component_t{std::move(locations), std::move(edges), "L1"}});
    return ntta_t{state_t{components, symbols}};
}

TEST_CASE("givenValidData_whenEvaluating_thenNoError", "[ntta]") {
    auto tta = generate_test_main_tta();
    REQUIRE(tta.state.components.begin()->second.current_location == "L1");
    REQUIRE(std::get<int>(tta.state.symbols["a"]) == 0);
    REQUIRE(std::get<bool>(tta.state.symbols["b"]) == false);
    std::cout << state_json << tta << std::endl;
    tta.tick();
    REQUIRE(tta.state.components.begin()->second.current_location == "L2");
    REQUIRE(std::get<int>(tta.state.symbols["a"]) == 0);
    REQUIRE(std::get<bool>(tta.state.symbols["b"]) == true);
    std::cout << state_json << tta << std::endl;
}
