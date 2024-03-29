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
#include "expr-wrappers/interpreter.h"
#include "symbol_table.h"
#include <ntta/interesting_tocker.h>
#include <ntta/async_tocker.h>
#include <catch2/catch_test_macros.hpp>

SCENARIO("interesting tocker", "[interesting-tocker]") {
    spdlog::set_level(spdlog::level::trace);
    aaltitoad::ntta_t::tta_map_t component_map{};
    expr::symbol_table_t external_symbols{};
    aaltitoad::expression_driver compiler{external_symbols};
    GIVEN("two external symbols and two TTA with edges guarding the respective external symbols") {
        external_symbols["x"] = false;
        external_symbols["y"] = false;
        { // TTA A
            auto factory = aaltitoad::tta_t::graph_builder{};
            factory.add_nodes({{"L0"},{"L1"}});
            factory.add_edge("L0", "L1", {.identifier="a", .guard=compiler.parse_guard("x"), .updates={}});
            component_map["A"] = {std::move(factory.build_heap()), "L0"};
        }
        { // TTA B
            auto factory = aaltitoad::tta_t::graph_builder{};
            factory.add_nodes({{"L0"},{"L1"}});
            factory.add_edge("L0", "L1", {.identifier="b", .guard=compiler.parse_guard("y"), .updates={}});
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
        auto emptyguard = compiler.parse_guard("");
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
            THEN("no change is available") {
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
            factory.add_edge("L0", "L1", {.identifier="a", .guard=compiler.parse_guard("x && !y"), .updates={}});
            component_map["A"] = {std::move(factory.build_heap()), "L0"};
        }
        { // TTA B
            auto factory = aaltitoad::tta_t::graph_builder{};
            factory.add_nodes({{"L0"},{"L1"}});
            factory.add_edge("L0", "L1", {.identifier="b", .guard=compiler.parse_guard("y && !x"), .updates={}});
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
            factory.add_edge("L0", "L1", {.identifier="a", .guard=compiler.parse_guard("x && !y"), .updates={}});
            component_map["A"] = {std::move(factory.build_heap()), "L0"};
        }
        { // TTA B
            auto factory = aaltitoad::tta_t::graph_builder{};
            factory.add_nodes({{"L0"},{"L1"}});
            factory.add_edge("L0", "L1", {.identifier="b", .guard=compiler.parse_guard("y && !x"), .updates={}});
            component_map["B"] = {std::move(factory.build_heap()), "L0"};
        }
        auto n = aaltitoad::ntta_t{symbols, {}, component_map};
        WHEN("calculating tock changes") {
            n.add_tocker(std::make_unique<aaltitoad::interesting_tocker>());
            auto changes = n.tock();
            THEN("no change is available") {
                REQUIRE(changes.empty());
            }
        }
    }
}

SCENARIO("dummy asynchronous tocker", "[dummy-async-tocker]") {
    class dummy_async_tocker : public aaltitoad::async_tocker_t {
        expr::symbol_table_t tock_values{};
        bool values_available = false;
        bool cancelled = false;
        auto get_tock_values(const expr::symbol_table_t& environment) -> expr::symbol_table_t {
            while(!values_available) {
                if(cancelled)
                    throw std::runtime_error("cancelled");
            }
            values_available = false;
            return tock_values;
        }
        void tock_async(const expr::symbol_table_t& env) override {
            job = std::async([this, &env](){
                return get_tock_values(env);
            });
        }
    public:
        ~dummy_async_tocker() override {
            cancelled = true;
        }
        void set_values(expr::symbol_table_t new_values) {
            tock_values = std::move(new_values);
            values_available = true;
        }
        // used for assertion purposes only
        void wait_for_ready(int max_ms) {
            if(job.wait_for(std::chrono::milliseconds(max_ms)) != std::future_status::ready)
                REQUIRE(false);
        }
    };
    spdlog::set_level(spdlog::level::trace);
    aaltitoad::ntta_t::tta_map_t component_map{};
    expr::symbol_table_t external_symbols{};
    aaltitoad::expression_driver compiler{external_symbols};
    GIVEN("a manually controllable dummy async tocker and one irrelevant TTA") {
        expr::symbol_table_t symbols{};
        symbols["x"] = false;
        { // TTA A
            auto factory = aaltitoad::tta_t::graph_builder{};
            factory.add_nodes({{"L0"}});
            component_map["A"] = {std::move(factory.build_heap()), "L0"};
        }
        auto n = aaltitoad::ntta_t{symbols, {}, component_map};
        auto tocker_instance = std::make_shared<dummy_async_tocker>();
        n.add_tocker(tocker_instance);
        WHEN("performing tocks without finishing the async task") {
            auto changes = n.tock();
            THEN("no changes are available") {
                REQUIRE(changes.empty());
            }
        }
        WHEN("performing tocks after finishing the async task") {
            auto changes = n.tock();
            REQUIRE(changes.empty());
            expr::symbol_table_t values{};
            values["x"] = 3;
            tocker_instance->set_values(values);
            tocker_instance->wait_for_ready(500);
            changes = n.tock();
            THEN("changes are available") {
                REQUIRE(1 == changes.size());
            }
        }
    }
}
