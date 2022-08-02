#include <runtime/tta/interesting_tocker.h>
#include <runtime/tta/async_tocker.h>
#include <catch2/catch_test_macros.hpp>

SCENARIO("interesting tocker", "[interesting-tocker]") {
    spdlog::set_level(spdlog::level::trace);
    aaltitoad::ntta_t::tta_map_t component_map{};
    expr::symbol_table_t external_symbols{};
    expr::compiler compiler{external_symbols};
    auto compile_guard = [&compiler](const std::string& guard) { compiler.trees = {}; compiler.parse(guard); return compiler.trees["expression_result"]; };
    GIVEN("two external symbols and two TTA with edges guarding the respective external symbols") {
        external_symbols["x"] = false;
        external_symbols["y"] = false;
        { // TTA A
            auto factory = aaltitoad::tta_t::graph_builder{};
            factory.add_nodes({{"L0"},{"L1"}});
            factory.add_edge("L0", "L1", {.identifier="a", .guard=compile_guard("x"), .updates={}});
            component_map["A"] = {std::move(factory.build_heap()), "L0"};
        }
        { // TTA B
            auto factory = aaltitoad::tta_t::graph_builder{};
            factory.add_nodes({{"L0"},{"L1"}});
            factory.add_edge("L0", "L1", {.identifier="b", .guard=compile_guard("y"), .updates={}});
            component_map["B"] = {std::move(factory.build_heap()), "L0"};
        }
        auto n = aaltitoad::ntta_t{{}, external_symbols, component_map};
        WHEN("adding the interesting_tocker") {
            n.add_tocker(std::make_unique<aaltitoad::interesting_tocker>());
            THEN("tocker is added to the network") {
                REQUIRE(1 == n.tockers.size());
            }
        }
        WHEN("calculating tock changes") {
            n.add_tocker(std::make_unique<aaltitoad::interesting_tocker>());
            auto changes = n.tock();
            THEN("four choices are available") {
                bool found1{}, found2{}, found3{}, found4{};
                REQUIRE(4 == changes.size());
                for(auto& change : changes) {
                    found1 |=  std::get<bool>(change["x"]) &&  std::get<bool>(change["y"]);
                    found2 |=  std::get<bool>(change["x"]) && !std::get<bool>(change["y"]);
                    found3 |= !std::get<bool>(change["x"]) &&  std::get<bool>(change["y"]);
                    found4 |= !std::get<bool>(change["x"]) && !std::get<bool>(change["y"]);
                }
                REQUIRE(found1);
                REQUIRE(found2);
                REQUIRE(found3);
                REQUIRE(found4);
            }
        }
    }
    GIVEN("two external symbols and no guards that checks them") {
        external_symbols["x"] = false;
        external_symbols["y"] = false;
        auto emptyguard = compile_guard("");
        { // TTA A
            auto factory = aaltitoad::tta_t::graph_builder{};
            factory.add_nodes({{"L0"},{"L1"}});
            factory.add_edge("L0", "L1", {.identifier="a", .guard=emptyguard, .updates={}});
            component_map["A"] = {std::move(factory.build_heap()), "L0"};
        }
        { // TTA B
            auto factory = aaltitoad::tta_t::graph_builder{};
            factory.add_nodes({{"L0"},{"L1"}});
            factory.add_edge("L0", "L1", {.identifier="b", .guard=emptyguard, .updates={}});
            component_map["B"] = {std::move(factory.build_heap()), "L0"};
        }
        auto n = aaltitoad::ntta_t{{}, external_symbols, component_map};
        WHEN("calculating tock changes") {
            n.add_tocker(std::make_unique<aaltitoad::interesting_tocker>());
            auto changes = n.tock();
            THEN("no changes are available") {
                REQUIRE(changes.empty());
            }
        }
    }
    GIVEN("two external symbols and two TTAs guard the symbols, where forcing both guards to be true is impossible") {
        external_symbols["x"] = false;
        external_symbols["y"] = false;
        { // TTA A
            auto factory = aaltitoad::tta_t::graph_builder{};
            factory.add_nodes({{"L0"},{"L1"}});
            factory.add_edge("L0", "L1", {.identifier="a", .guard=compile_guard("x && !y"), .updates={}});
            component_map["A"] = {std::move(factory.build_heap()), "L0"};
        }
        { // TTA B
            auto factory = aaltitoad::tta_t::graph_builder{};
            factory.add_nodes({{"L0"},{"L1"}});
            factory.add_edge("L0", "L1", {.identifier="b", .guard=compile_guard("y && !x"), .updates={}});
            component_map["B"] = {std::move(factory.build_heap()), "L0"};
        }
        auto n = aaltitoad::ntta_t{{}, external_symbols, component_map};
        WHEN("calculating tock changes") {
            n.add_tocker(std::make_unique<aaltitoad::interesting_tocker>());
            REQUIRE(1 == n.tockers.size());
            REQUIRE(n.symbols.empty());
            REQUIRE(2 == n.external_symbols.size());
            auto changes = n.tock();
            THEN("three choices are available") {
                REQUIRE(3 == changes.size());
            }
        }
    }
    GIVEN("two internal symbols and two TTAs guard the symbols") {
        expr::symbol_table_t symbols{};
        symbols["x"] = false;
        symbols["y"] = false;
        { // TTA A
            auto factory = aaltitoad::tta_t::graph_builder{};
            factory.add_nodes({{"L0"},{"L1"}});
            factory.add_edge("L0", "L1", {.identifier="a", .guard=compile_guard("x && !y"), .updates={}});
            component_map["A"] = {std::move(factory.build_heap()), "L0"};
        }
        { // TTA B
            auto factory = aaltitoad::tta_t::graph_builder{};
            factory.add_nodes({{"L0"},{"L1"}});
            factory.add_edge("L0", "L1", {.identifier="b", .guard=compile_guard("y && !x"), .updates={}});
            component_map["B"] = {std::move(factory.build_heap()), "L0"};
        }
        auto n = aaltitoad::ntta_t{symbols, {}, component_map};
        WHEN("calculating tock changes") {
            n.add_tocker(std::make_unique<aaltitoad::interesting_tocker>());
            auto changes = n.tock();
            THEN("no choices are available") {
                REQUIRE(changes.empty());
            }
        }
    }
}

SCENARIO("dummy asynchronous tocker", "[dummy-async-tocker]") {
    GIVEN("a manually controllable dummy async tocker") {
        WHEN("performing tocks without finishing the async task") {
            THEN("no changes are available") {

            }
        }
        WHEN("performing tocks after finishing the async task") {
            THEN("changes are available") {

            }
        }
    }
}
