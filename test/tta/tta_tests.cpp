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
#include "driver/evaluator.h"
#include "expr-wrappers/interpreter.h"
#include "symbol_table.h"
#include <ntta/tta.h>
#include <catch2/catch_test_macros.hpp>
#include <utility>

SCENARIO("djwkadjw", "[aa]") {
    expr::symbol_table_t ex_symbols{};
    aaltitoad::expression_driver i{ex_symbols};
    auto r = i.parse("x:=0_ms");
    std::cout << r.declarations << std::endl;
}

SCENARIO("constructing networks of TTAs", "[ntta_t-construction]") {
    struct dummy_tocker : public aaltitoad::tocker_t {
        [[nodiscard]] auto tock(const aaltitoad::ntta_t& state) -> std::vector<expr::symbol_table_t> override {
            return changes;
        }
        dummy_tocker() : changes{} {}
        explicit dummy_tocker(std::vector<expr::symbol_table_t> changes) : changes{std::move(changes)} {}
        ~dummy_tocker() override = default;
        std::vector<expr::symbol_table_t> changes{};
    };
    spdlog::set_level(spdlog::level::trace);
    aaltitoad::ntta_t::tta_map_t component_map{};
    expr::symbol_table_t symbols{};
    aaltitoad::expression_driver compiler{symbols};
    auto compile_update = [&compiler](const std::string& updates) -> expr::syntax_tree_collection_t { return compiler.parse(updates).declarations; };
    auto empty_guard = compiler.parse_guard("");
    GIVEN("two TTAs with no enabled edges") {
        symbols["x"] = 0;
        { // TTA A
            auto factory = aaltitoad::tta_t::graph_builder{};
            factory.add_nodes({{"L0"},{"L1"}});
            factory.add_edge("L0", "L1", {.identifier="a", .guard=compiler.parse_guard("x > 0"), .updates={}});
            component_map["A"] = {std::move(factory.build_heap()), "L0"};
        }
        { // TTA B
            auto factory = aaltitoad::tta_t::graph_builder{};
            factory.add_nodes({{"L0"},{"L1"}});
            factory.add_edge("L0", "L1", {.identifier="b", .guard=compiler.parse_guard("x > 0"), .updates={}});
            component_map["B"] = {std::move(factory.build_heap()), "L0"};
        }
        WHEN("trying to construct") {
            auto n = aaltitoad::ntta_t{symbols, component_map};
            THEN("no error occurs") {
                REQUIRE(2 == n.components.size());
                REQUIRE(n.components.contains("A"));
                REQUIRE(n.components.contains("B"));
            }
        }
    }
    GIVEN("no TTAs") {
        auto n = aaltitoad::ntta_t{};
        THEN("no error occurs no components are registered") {
            REQUIRE(n.components.empty());
        }
        WHEN("trying to tick") {
            THEN("no error and no changes") {
                auto changes = n.tick();
                REQUIRE(changes.empty());
            }
        }
        GIVEN("adding a tocker implementation with no changes to report") {
            n.add_tocker(std::make_unique<dummy_tocker>());
            THEN("tocker is added to the list") {
                REQUIRE(1 == n.tockers.size());
            }
            WHEN("performing a tock") {
                auto tock_changes = n.tock();
                THEN("no changes are calculated") {
                    REQUIRE(tock_changes.empty());
                }
            }
        }
        GIVEN("adding a tocker implementation with some changes to report") {
            expr::symbol_table_t ex_symbols{};
            n.external_symbols["x"] = 0;
            aaltitoad::expression_driver i{ex_symbols};
            auto interpret_update = [&i](const std::string& update) -> expr::symbol_table_t { return i.parse(update).get_symbol_table(); };
            n.add_tocker(std::make_unique<dummy_tocker>(std::vector<expr::symbol_table_t>{interpret_update("x:=32")}));
            THEN("tocker is added to the list") {
                REQUIRE(1 == n.tockers.size());
            }
            WHEN("calculating tock changes") {
                auto tock_changes = n.tock();
                THEN("the changes are propagated") {
                    REQUIRE(1 == tock_changes.size());
                    for(auto& change : tock_changes) {
                        REQUIRE(change.contains("x"));
                        REQUIRE(32 == std::get<int>(change["x"]));
                    }
                }
                THEN("only external variables are touched") {
                    for(auto& change : tock_changes) {
                        for(auto& var : change)
                            REQUIRE_FALSE(symbols.contains(var.first));
                    }
                }
                WHEN("applying those changes") {
                    n.apply(tock_changes[0]);
                    THEN("only external variables are touched") {
                        for(auto& change : tock_changes) {
                            for(auto& var : change) {
                                REQUIRE_FALSE(n.symbols.contains(var.first));
                                REQUIRE(n.external_symbols.contains(var.first));
                            }
                        }
                    }
                }
            }
        }
    }
    GIVEN("two TTAs with 1 enabled edge each") {
        symbols["x"] = 0;
        { // TTA A
            auto factory = aaltitoad::tta_t::graph_builder{};
            factory.add_nodes({{"L0"},{"L1"}});
            factory.add_edge("L0", "L1", {.identifier="a", .guard=compiler.parse_guard("x >= 0"), .updates={}});
            component_map["A"] = {std::move(factory.build_heap()), "L0"};
        }
        { // TTA B
            auto factory = aaltitoad::tta_t::graph_builder{};
            factory.add_nodes({{"L0"},{"L1"}});
            factory.add_edge("L0", "L1", {.identifier="b", .guard=compiler.parse_guard("x >= 0"), .updates={}});
            component_map["B"] = {std::move(factory.build_heap()), "L0"};
        }
        auto n = aaltitoad::ntta_t{symbols, component_map};
        WHEN("serializing to string") {
            std::stringstream ss{};
            ss << n;
            THEN("no error occurs and string is printable") {
                spdlog::debug(ss.str()); // manually check that this output is understandable for humans to read
            }
        }
        WHEN("hashing the current state of the network") {
            auto initial_state_hash = std::hash<aaltitoad::ntta_t>{}(n);
            THEN("hashes of same state is the same") {
                auto same_hash = std::hash<aaltitoad::ntta_t>{}(n);
                REQUIRE(initial_state_hash == same_hash);
            }
            WHEN("applying some changes and hashing the new state") {
                auto changes = n.tick();
                REQUIRE_FALSE(changes.empty());
                n.apply(changes[0]);
                auto new_state_hash = std::hash<aaltitoad::ntta_t>{}(n);
                THEN("hashes of the different states are different") {
                    REQUIRE(new_state_hash != initial_state_hash);
                }
            }
        }
    }
    GIVEN("invalid control flow graph initial state") {
        auto factory = aaltitoad::tta_t::graph_builder{};
        factory.add_nodes({{"L0"},{"L1"}});
        factory.add_edge("L0", "L1", {.identifier="a", .guard=compiler.parse_guard("x > 0"), .updates={}});
        WHEN("constructing TTA") {
            THEN("out_of_range exception is thrown") {
                auto ex = [&](){ component_map["A"] = {std::move(factory.build_heap()), "not a valid initial state"}; };
                REQUIRE_THROWS_AS(ex(), std::out_of_range);
            }
        }
    }
}

SCENARIO("ticking result in maximal behavior (no tockers registered)", "[tick-maximal-no-tockers]") {
    spdlog::set_level(spdlog::level::trace);
    aaltitoad::ntta_t::tta_map_t component_map{};
    expr::symbol_table_t symbols{};
    aaltitoad::expression_driver compiler{symbols};
    auto compile_update = [&compiler](const std::string& updates) -> expr::syntax_tree_collection_t { return compiler.parse(updates).declarations; };
    auto empty_guard = compiler.parse_guard("");
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
    GIVEN("two TTAs with 1 disabled edge each no update overlap") {
        symbols["x"] = 0;
        { // TTA A
            auto factory = aaltitoad::tta_t::graph_builder{};
            factory.add_nodes({{"L0"},{"L1"}});
            factory.add_edge("L0", "L1", {.identifier="a", .guard=compiler.parse_guard("x > 1"), .updates={}});
            component_map["A"] = {std::move(factory.build_heap()), "L0"};
        }
        { // TTA B
            auto factory = aaltitoad::tta_t::graph_builder{};
            factory.add_nodes({{"L0"},{"L1"}});
            factory.add_edge("L0", "L1", {.identifier="b", .guard=compiler.parse_guard("x != 0"), .updates={}});
            component_map["B"] = {std::move(factory.build_heap()), "L0"};
        }
        auto n = aaltitoad::ntta_t{symbols, component_map};
        WHEN("calculating tick changes") {
            auto changes = n.tick();
            THEN("no changes are available") {
                REQUIRE(changes.empty());
            }
        }
    }
    GIVEN("one TTAs checking an external variable") {
        expr::symbol_table_t ex_symbols{};
        ex_symbols["x"] = 0;
        aaltitoad::expression_driver ex_compiler{ex_symbols};
        { // TTA A
            auto factory = aaltitoad::tta_t::graph_builder{};
            factory.add_nodes({{"L0"},{"L1"}});
            factory.add_edge("L0", "L1", {.identifier="a", .guard=ex_compiler.parse_guard("x >= 0"), .updates={}});
            component_map["A"] = {std::move(factory.build_heap()), "L0"};
        }
        auto n = aaltitoad::ntta_t{symbols, ex_symbols, component_map};
        WHEN("calculating tick changes") {
            auto changes = n.tick();
            THEN("one change is available") {
                REQUIRE(1 == changes.size());
            }
        }
    }
}
