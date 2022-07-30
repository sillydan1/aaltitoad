#include <runtime/tta/tta.h>
#include <catch2/catch_test_macros.hpp>
#include <utility>
#include <runtime/tta/interesting_tocker.h>

// TODO: Ctor scenario with tocker adding (just a dummy tocker)
// TODO: serialization scenario (not much to assert on, but good resource for lookup)
// TODO: hashing scenario
// TODO: async / sync tocker scenario

SCENARIO("ticking result in maximal behavior (no tockers registered)", "[tick-maximal-no-tockers]") {
    spdlog::set_level(spdlog::level::debug);
    aaltitoad::ntta_t::tta_map_t component_map{};
    expr::symbol_table_t symbols{}; // TODO: How do we handle external symbols here?
    expr::compiler compiler{symbols};
    auto compile_update = [&compiler](const std::string& updates) { compiler.trees = {}; compiler.parse(updates); return compiler.trees; };
    auto compile_guard = [&compiler](const std::string& guard) { compiler.trees = {}; compiler.parse(guard); return compiler.trees["expression_result"]; };
    auto empty_guard = compile_guard("");
    GIVEN("two TTAs with 1 enabled edge no update overlap") {
        { // TTA A
            auto factory = aaltitoad::tta_t::graph_builder{};
            factory.add_nodes({{"L0"},{"L1"}});
            factory.add_edge("L0", "L1", {.identifier="a", .guard=empty_guard, .updates={}});
            component_map["A"] = {std::move(factory.build_heap()), "L0"};
        }
        { // TTA B
            auto factory = aaltitoad::tta_t::graph_builder{};
            factory.add_nodes({{"L0"},{"L1"}});
            factory.add_edge("L0", "L1", {.identifier="b", .guard=empty_guard, .updates={}});
            component_map["B"] = {std::move(factory.build_heap()), "L0"};
        }
        auto n = aaltitoad::ntta_t{symbols, component_map};
        WHEN("calculating tick changes") {
            auto changes = n.tick();
            THEN("only one maximal tick solution is available") {
                REQUIRE(1 == changes.size());
                REQUIRE(changes[0].symbol_changes.empty());
                REQUIRE(2 == changes[0].location_changes.size());
                REQUIRE("L1" == changes[0].location_changes[0].new_location->first);
            }
        }
        WHEN("calculating tock changes") {
            auto changes = n.tock();
            THEN("no changes are generated (no tockers registered)") {
                REQUIRE(changes.empty());
            }
        }
        WHEN("applying tick changes") {
            auto changes = n.tick();
            n.apply(changes[0]);
            THEN("the components' current location has changed") {
                for(auto& component : n.components)
                    REQUIRE("L1" == component.second.current_location->first);
            }
        }
    }
    GIVEN("four TTAs with many edges and some update overlap") {
        symbols["x"] = 0; symbols["y"] = 0; symbols["z"] = 0;
        { // A
            auto factory = aaltitoad::tta_t::graph_builder{};
            factory.add_nodes({{"L0"},{"L1"}});
            factory.add_edge("L0", "L1", {.identifier="a", .guard=empty_guard, .updates=compile_update("x:=1")});
            factory.add_edge("L0", "L1", {.identifier="d", .guard=empty_guard, .updates={}});
            factory.add_edge("L0", "L1", {.identifier="e", .guard=empty_guard, .updates=compile_update("y:=1")});
            component_map["A"] = {std::move(factory.build_heap()), "L0"};
        }
        { // B
            auto factory = aaltitoad::tta_t::graph_builder{};
            factory.add_nodes({{"L0"},{"L1"}});
            factory.add_edge("L0", "L1", {.identifier="b", .guard=empty_guard, .updates=compile_update("x:=2")});
            factory.add_edge("L0", "L1", {.identifier="c", .guard=empty_guard, .updates={}});
            factory.add_edge("L0", "L1", {.identifier="f", .guard=empty_guard, .updates=compile_update("z:=1")});
            component_map["B"] = {std::move(factory.build_heap()), "L0"};
        }
        { // C
            auto factory = aaltitoad::tta_t::graph_builder{};
            factory.add_nodes({{"L0"},{"L1"}});
            factory.add_edge("L0", "L1", {.identifier="g", .guard=empty_guard, .updates={}});
            factory.add_edge("L0", "L1", {.identifier="h", .guard=empty_guard, .updates=compile_update("y:=2;z:=2")});
            component_map["C"] = {std::move(factory.build_heap()), "L0"};
        }
        { // D
            auto factory = aaltitoad::tta_t::graph_builder{};
            factory.add_nodes({{"L0"},{"L1"}});
            factory.add_edge("L0", "L1", {.identifier="i", .guard=empty_guard, .updates=compile_update("z:=3")});
            component_map["D"] = {std::move(factory.build_heap()), "L0"};
        }
        auto n = aaltitoad::ntta_t{symbols, component_map};
        WHEN("calculating tick changes") {
            auto changes = n.tick();
            THEN("the all maximal solutions are found") {
                REQUIRE(6 == changes.size());
            }
        }
        WHEN("calculating tock changes") {
            auto changes = n.tock();
            THEN("no changes are generated (no tockers registered)") {
                REQUIRE(changes.empty());
            }
        }
    }
    GIVEN("two TTAs with conflicting update overlap") {
        { // A
            auto factory = aaltitoad::tta_t::graph_builder{};
            factory.add_nodes({{"L0"},{"L1"}});
            factory.add_edge("L0", "L1", {.identifier="a", .guard=empty_guard, .updates=compile_update("x:=1")});
            component_map["A"] = {std::move(factory.build_heap()), "L0"};
        }
        { // B
            auto factory = aaltitoad::tta_t::graph_builder{};
            factory.add_nodes({{"L0"},{"L1"}});
            factory.add_edge("L0", "L1", {.identifier="b", .guard=empty_guard, .updates=compile_update("x:=2")});
            component_map["B"] = {std::move(factory.build_heap()), "L0"};
        }
        auto n = aaltitoad::ntta_t{symbols, component_map};
        WHEN("calculating tick changes") {
            auto changes = n.tick();
            THEN("all two maximal solutions are found") {
                REQUIRE(2 == changes.size());
                bool found_a = false, found_b = false, found_nonsense = false;
                for(auto& change : changes) {
                    found_a |= std::get<bool>(ee_(change.symbol_changes["x"],1));
                    found_b |= std::get<bool>(ee_(change.symbol_changes["x"],2));
                    found_nonsense |= std::get<bool>(ee_(change.symbol_changes["x"],3));
                }
                REQUIRE(found_a);
                REQUIRE(found_b);
                REQUIRE(!found_nonsense);
            }
        }
    }
}
