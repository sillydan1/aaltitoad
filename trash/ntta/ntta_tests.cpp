#include "../../trash/ntta.h"
#include <catch2/catch_test_macros.hpp>
#include <utility>

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
//// You can override the edge's guard, update and target location by
//// providing a test_tta_edge_defines struct.
struct test_tta_edge_defines {
    std::string target = "L2";
    std::string g1 = "a >= 0";
    std::string u1 = "b := true";
};
ntta_t generate_test_main_tta(const test_tta_edge_defines& defines = {}) {
    expr::symbol_table_t symbols{};
    symbols["a"] = 0;
    symbols["b"] = false;
    location_map_t locations{};
    locations.insert({"L1", location_t{}});
    locations.insert({"L2", location_t{}});
    edge_list_t edges{};
    edges.emplace_back("L1", defines.target, defines.g1, defines.u1);
    component_map_t components{};
    components.insert({"Main", component_t{std::move(locations), std::move(edges), "L1"}});
    auto tta = ntta_t{state_t{components, symbols}};
    REQUIRE(tta.state.components.begin()->second.current_location == "L1");
    REQUIRE(std::get<int>(tta.state.symbols["a"]) == 0);
    REQUIRE(std::get<bool>(tta.state.symbols["b"]) == false);
    return tta;
}

TEST_CASE("givenNonExistentInitialLocation_whenConstructingComponent_thenThrow") {
    REQUIRE_THROWS_AS(component_t({{"L1", location_t{}}}, {}, "L2"), std::logic_error);
}

TEST_CASE("givenValidData_whenEvaluatingOneTick_thenNoError") {
    auto tta = generate_test_main_tta();
    tta.tick();
    REQUIRE(tta.state.components.begin()->second.current_location == "L2");
    REQUIRE(std::get<int>(tta.state.symbols["a"]) == 0);
    REQUIRE(std::get<bool>(tta.state.symbols["b"]) == true);
}

TEST_CASE("givenValidData_whenEvaluatingMoreTicks_thenNoError") {
    auto tta = generate_test_main_tta();
    tta.tick();
    REQUIRE(tta.state.components.begin()->second.current_location == "L2");
    REQUIRE(std::get<int>(tta.state.symbols["a"]) == 0);
    REQUIRE(std::get<bool>(tta.state.symbols["b"]) == true);
    tta.tick();
    tta.tick();
    tta.tick(); // Since the tta tick-stalls, additional ticks will not change the state.
    REQUIRE(tta.state.components.begin()->second.current_location == "L2");
    REQUIRE(std::get<int>(tta.state.symbols["a"]) == 0);
    REQUIRE(std::get<bool>(tta.state.symbols["b"]) == true);
}

TEST_CASE("givenValidData_whenOneTickAndTock_thenNoError") {
    auto tta = generate_test_main_tta();
    tta.tick();
    REQUIRE(tta.state.components.begin()->second.current_location == "L2");
    REQUIRE(std::get<int>(tta.state.symbols["a"]) == 0);
    REQUIRE(std::get<bool>(tta.state.symbols["b"]) == true);
    tta.tock(); // There's nothing external here, so nothing should happen
    REQUIRE(tta.state.components.begin()->second.current_location == "L2");
    REQUIRE(std::get<int>(tta.state.symbols["a"]) == 0);
    REQUIRE(std::get<bool>(tta.state.symbols["b"]) == true);
}

TEST_CASE("givenValidData_whenSerializedAsHuman_thenHumanFormatIsGenerated") {
    auto tta = generate_test_main_tta();
    std::stringstream ss{};
    ss << tta;
    REQUIRE(ss.str() == "State: {\n"
                        "locations\n"
                        "[\n"
                        "    L1,\n"
                        "]\n"
                        "symbols\n"
                        "<\n"
                        "a :-> 0 i\n"
                        "b :-> false b\n"
                        ">}");
}

TEST_CASE("givenValidData_whenSerializedAsMachineJson_thenMachineJsonIsGenerated") {
    auto tta = generate_test_main_tta();
    std::stringstream ss{};
    ss << stream_mods::json << tta;
    REQUIRE(ss.str() == "{\"locations\":{\"Main\":\"L1\"},\"symbols\":{\"a\":0,\"b\":false}}");
}

TEST_CASE("givenNewTypeInUpdate_whenEvaluatingOneTick_thenDynamicallyChangeType") {
    // TODO: Is this a feature? If not, this test should be a "REQUIRE_THROWS_AS(tta.tick(), logic_error)" assertion
    auto tta = generate_test_main_tta({.u1="b := 3"});
    REQUIRE_NOTHROW(tta.tick());
    REQUIRE(std::get<int>(tta.state.symbols["b"]) == 3);
    REQUIRE_THROWS_AS(std::get<bool>(tta.state.symbols["b"]), std::bad_variant_access); // b's type have changed
}

TEST_CASE("givenInvalidGuard_whenEvaluatingOneTick_thenFails") {
    auto tta = generate_test_main_tta({.g1="a == false"});
    REQUIRE_THROWS_AS(tta.tick(), std::logic_error);
}

TEST_CASE("givenInvalidTargetLocation_whenEvaluatingOneTick_thenFails") {
    auto tta = generate_test_main_tta({.target="L3"});
    REQUIRE_THROWS_AS(tta.tick(), std::logic_error);
}

// TODO: Test is disabled due to infinite loop in expr grammar
TEST_CASE("givenInvalidGuardSyntax_whenEvaluatingOneTick_thenFails", "[.]") {
    auto tta = generate_test_main_tta({.g1="not a guard"});
    REQUIRE_THROWS_AS(tta.tick(), std::logic_error);
}

// TODO: Test is disabled due to infinite loop in expr grammar
TEST_CASE("givenInvalidUpdateSyntax_whenEvaluatingOneTick_thenFails", "[.]") {
    auto tta = generate_test_main_tta({.u1="not an update"});
    REQUIRE_THROWS_AS(tta.tick(), std::logic_error);
}

#include <runtime/tta/tta.h>
struct n_digit_num_t {
    std::vector<size_t> max_digits;
    std::vector<size_t> number;
    n_digit_num_t(std::vector<size_t>&& m, std::vector<size_t>&& n) : max_digits{std::move(m)}, number{std::move(n)} {}
    void operator++() {
        for(size_t i = number.size()-1; i >= 0; i--) {
            if(number[i] < max_digits[i]) {
                number[i]++;
                break;
            }
            number[i] = 0;
        }
    }
};
template<typename T, typename R>
auto generate_permutations(const std::vector<std::vector<T>>& input, std::function<R(const std::vector<typename std::vector<T>::const_iterator>&)>& combiner) -> std::vector<R> {
    std::vector<size_t> m{}; m.reserve(input.size());
    std::vector<size_t> n{}; n.reserve(input.size());
    for(auto& v : input) {
        n.push_back(0);
        if(v.empty()) {
            m.push_back(0);
            continue;
        }
        m.push_back(v.size()-1);
    }
    auto plus_one_mult = [](const size_t& a, const size_t& b){ return a * (b+1); };
    auto max_num = std::accumulate(m.begin(), m.end(), 1, plus_one_mult);
    n_digit_num_t tt{std::move(m),std::move(n)};
    std::vector<R> result{}; result.reserve(max_num);
    std::vector<typename std::vector<T>::const_iterator> c{}; c.reserve(tt.number.size());
    for(int i = 0; i < max_num; i++) {
        c.clear();
        for(size_t j = 0; j < tt.number.size(); j++)
            c.push_back(input[j].begin() + tt.number[j]);
        result.push_back(combiner(c));
        ++tt;
    }
    return result;
}

#include <sstream>
TEST_CASE("modularized_tta_test") {
    std::function<std::string(const std::vector<typename std::vector<int>::const_iterator>&)> f = [](const std::vector<typename std::vector<int>::const_iterator>& n){
        std::stringstream ss{};
        for(auto& x : n)
            ss << *x;
        return ss.str();
    };
    auto result = generate_permutations<int,std::string>({{0,1},{0,1},{0,1},{0,1}}, f);
    for(auto& r : result)
        std::cout << r << std::endl;
    std::cout << "e";
}
