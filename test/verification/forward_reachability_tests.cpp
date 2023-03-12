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
#include "expr-wrappers/ctl-interpreter.h"
#include <ntta/tta.h>
#include <catch2/catch_test_macros.hpp>
#include <ntta/builder/ntta_builder.h>
#include <verification/forward_reachability.h>

SCENARIO("basic reachability", "[frs]") {
    spdlog::set_level(spdlog::level::trace);
    GIVEN("one tta with a simple count-down loop") {
        aaltitoad::ntta_builder builder{};
        aaltitoad::expression_driver compiler{builder.symbols, builder.external_symbols};
        auto n = builder
                // Add symbols
                .add_symbols({{"x", 5}})
                // Add components
                .add_tta("A", aaltitoad::tta_builder{&compiler}
                        .add_locations({"L0", "L1"})
                        .set_starting_location("L0")
                        .add_edges({{"L0", "L1", "x > 0", "x := x - 1"}, {"L1", "L0"}}))
                // Add tockers
                .build_with_interesting_tocker();
        GIVEN("a simple reachability query 'can x reach zero?'") {
            auto query = aaltitoad::ctl_interpreter{n.symbols, n.external_symbols}.compile("E F x == 0");
            WHEN("searching through the state-space with forward reachability search") {
                aaltitoad::forward_reachability_searcher frs{};
                auto results = frs.is_reachable(n, query);
                THEN("there is only one query in the answer") {
                    REQUIRE(results.size() == 1);
                }
                AND_THEN("the query is satisfied") {
                    for (auto& result: results)
                        REQUIRE(result.solution.has_value());
                }
                AND_THEN("'x' is 0 in the satisfaction state") {
                    REQUIRE(std::get<bool>(results.begin()->solution.value()->second.data.symbols.at("x") == 0));
                }
                AND_THEN("the trace is printable") {
                    std::cout << results.begin()->solution.value() << std::endl;
                }
            }
        }
        GIVEN("a simple reachability query 'can L1 be reached?'") {
            auto s = n.symbols + n.external_symbols;
            auto query = aaltitoad::ctl_interpreter{s}.compile("E F L1");
            WHEN("searching through the state-space with forward reachability search") {
                aaltitoad::forward_reachability_searcher frs{};
                auto results = frs.is_reachable(n, query);
                THEN("there is only one query in the answer") {
                    REQUIRE(results.size() == 1);
                }
                AND_THEN("the query is satisfied") {
                    REQUIRE(results.begin()->solution.has_value());
                }
                AND_THEN("L1 is the current state of the component in the sat state") {
                    auto sat_state_cur_loc = results.begin()->solution.value()->second.data.components["A"].current_location->first;
                    REQUIRE(sat_state_cur_loc == "L1");
                }
            }
        }
    }
    GIVEN("one looping tta and no symbol manipulation") {
        aaltitoad::ntta_builder builder{};
        aaltitoad::expression_driver compiler{builder.symbols, builder.external_symbols};
        auto n = builder
                .add_symbol({"x", 0})
                // Add components
                .add_tta("A", aaltitoad::tta_builder{&compiler}
                        .add_location("L0")
                        .set_starting_location("L0")
                        .add_edge({"L0", "L0"}))
                // Add tockers
                .build_with_interesting_tocker();
        GIVEN("a simple unsatisfiable query 'can x reach 1?'") {
            auto s = n.symbols + n.external_symbols;
            auto query = aaltitoad::ctl_interpreter{s}.compile("E F x == 1");
            WHEN("searching through the state-space with forward reachability search") {
                aaltitoad::forward_reachability_searcher frs{};
                auto results = frs.is_reachable(n, query);
                THEN("no answer can be found") {
                    REQUIRE(results.size() == 1);
                    REQUIRE(!results.begin()->solution.has_value());
                }
            }
        }
    }
    GIVEN("one tta with an interesting edge from initial location") {
        aaltitoad::ntta_builder builder{};
        aaltitoad::expression_driver compiler{builder.symbols, builder.external_symbols};
        auto n = builder
                // Add symbols
                .add_external_symbol({"y", 0})
                        // Add components
                .add_tta("A", aaltitoad::tta_builder{&compiler}
                        .add_locations({"L0", "L1"})
                        .set_starting_location("L0")
                        .add_edge({"L0", "L1", "y > 0", ""}))
                        // Add tockers
                .build_with_interesting_tocker();
        GIVEN("a simple query matching the interesting guard 'E F y > 0'") {
            auto s = n.symbols + n.external_symbols;
            auto query = aaltitoad::ctl_interpreter{s}.compile("E F y > 0");
            WHEN("searching through the state-space with forward reachability search") {
                aaltitoad::forward_reachability_searcher frs{};
                auto results = frs.is_reachable(n, query);
                THEN("the query is satisfied") {
                    REQUIRE(results.begin()->solution.has_value());
                    auto& y_sym = results.begin()->solution.value()->second.data.external_symbols.at("y");
                    REQUIRE(std::get<bool>(y_sym > expr::symbol_value_t{0}));
                }
            }
        }
    }
    GIVEN("one tta with an interesting edge from somewhere in the middle") {
        aaltitoad::ntta_builder builder{};
        aaltitoad::expression_driver compiler{builder.symbols, builder.external_symbols};
        auto n = builder
                // Add symbols
                .add_external_symbol({"y", 0})
                        // Add components
                .add_tta("A", aaltitoad::tta_builder{&compiler}
                        .add_locations({"L0", "L1", "L2"})
                        .set_starting_location("L0")
                        .add_edge({"L0", "L1"})
                        .add_edge({"L1", "L2"})
                        .add_edge({"L2", "L0", "y > 0", ""}))
                        // Add tockers
                .build_with_interesting_tocker();
        GIVEN("a simple query matching the interesting guard 'E F y > 0'") {
            auto s = n.symbols + n.external_symbols;
            auto query = aaltitoad::ctl_interpreter{s}.compile("E F y > 0");
            WHEN("searching through the state-space with forward reachability search (strategy last)") {
                aaltitoad::forward_reachability_searcher frs{aaltitoad::pick_strategy::last};
                auto results = frs.is_reachable(n, query);
                THEN("the query is satisfied") {
                    REQUIRE(results.begin()->solution.has_value());
                    auto& y_sym = results.begin()->solution.value()->second.data.external_symbols.at("y");
                    REQUIRE(std::get<bool>(y_sym > expr::symbol_value_t{0}));
                }
            }
        }
        GIVEN("is L2 reachable?") {
            auto s = n.symbols + n.external_symbols;
            auto query = aaltitoad::ctl_interpreter{s}.compile("E F L2");
            WHEN("searching through the state-space with forward reachability search") {
                aaltitoad::forward_reachability_searcher frs{aaltitoad::pick_strategy::random};
                auto results = frs.is_reachable(n, query);
                THEN("the query is satisfied") {
                    REQUIRE(results.begin()->solution.has_value());
                }
            }
        }
    }
}
