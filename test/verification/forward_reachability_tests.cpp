/**
 * aaltitoad - a verification engine for tick tock automata models
   Copyright (C) 2026 Asger Gitz-Johansen

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
// #include "cli/verifier/query/query_json_loader.h"
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

SCENARIO("safety queries are disproven by counterexamples", "[frs]") {
    GIVEN("one tta with a simple count-down loop from 5") {
        aaltitoad::ntta_builder builder{};
        aaltitoad::expression_driver compiler{builder.symbols, builder.external_symbols};
        auto n = builder
                .add_symbols({{"x", 5}})
                .add_tta("A", aaltitoad::tta_builder{&compiler}
                        .add_locations({"L0", "L1"})
                        .set_starting_location("L0")
                        .add_edges({{"L0", "L1", "x > 0", "x := x - 1"}, {"L1", "L0"}}))
                .build_with_interesting_tocker();
        auto s = n.symbols + n.external_symbols;
        GIVEN("an invariant that holds in every reachable state 'A G x >= 0'") {
            auto query = aaltitoad::ctl_interpreter{s}.compile("A G x >= 0");
            WHEN("searching through the state-space with forward reachability search") {
                aaltitoad::forward_reachability_searcher frs{};
                auto results = frs.is_reachable(n, query);
                THEN("the query is satisfied without a counterexample") {
                    REQUIRE(results.size() == 1);
                    REQUIRE(results.begin()->satisfied());
                    REQUIRE_FALSE(results.begin()->solution.has_value());
                }
            }
        }
        GIVEN("an invariant that is violated when x reaches zero 'A G x > 0'") {
            auto query = aaltitoad::ctl_interpreter{s}.compile("A G x > 0");
            WHEN("searching through the state-space with forward reachability search") {
                aaltitoad::forward_reachability_searcher frs{};
                auto results = frs.is_reachable(n, query);
                THEN("the query is not satisfied") {
                    REQUIRE(results.size() == 1);
                    REQUIRE_FALSE(results.begin()->satisfied());
                }
                AND_THEN("the counterexample is a state where x is 0") {
                    REQUIRE(results.begin()->solution.has_value());
                    REQUIRE(std::get<bool>(results.begin()->solution.value()->second.data.symbols.at("x") == 0));
                }
            }
        }
        GIVEN("an invariant on a location that is always left again 'A G L0'") {
            auto query = aaltitoad::ctl_interpreter{s}.compile("A G L0");
            WHEN("searching through the state-space with forward reachability search") {
                aaltitoad::forward_reachability_searcher frs{};
                auto results = frs.is_reachable(n, query);
                THEN("the query is not satisfied, and the counterexample is in L1") {
                    REQUIRE_FALSE(results.begin()->satisfied());
                    REQUIRE(results.begin()->solution.has_value());
                    REQUIRE(results.begin()->solution.value()->second.data.components["A"].current_location->first == "L1");
                }
            }
        }
        GIVEN("a reachability query and a safety query that are both satisfied") {
            std::vector<ctl::syntax_tree_t> queries{
                aaltitoad::ctl_interpreter{s}.compile("E F x == 0"),
                aaltitoad::ctl_interpreter{s}.compile("A G x <= 5")};
            WHEN("searching through the state-space with forward reachability search") {
                aaltitoad::forward_reachability_searcher frs{};
                auto results = frs.is_reachable(n, queries);
                THEN("both queries are satisfied") {
                    REQUIRE(results.size() == 2);
                    REQUIRE(results[0].satisfied());
                    REQUIRE(results[0].solution.has_value());
                    REQUIRE(results[1].satisfied());
                    REQUIRE_FALSE(results[1].solution.has_value());
                }
            }
        }
    }
    GIVEN("one looping tta and no symbol manipulation") {
        aaltitoad::ntta_builder builder{};
        aaltitoad::expression_driver compiler{builder.symbols, builder.external_symbols};
        auto n = builder
                .add_symbol({"x", 0})
                .add_tta("A", aaltitoad::tta_builder{&compiler}
                        .add_location("L0")
                        .set_starting_location("L0")
                        .add_edge({"L0", "L0"}))
                .build_with_interesting_tocker();
        GIVEN("an unreachable state 'E F x == 1'") {
            auto s = n.symbols + n.external_symbols;
            auto query = aaltitoad::ctl_interpreter{s}.compile("E F x == 1");
            WHEN("searching through the state-space with forward reachability search") {
                aaltitoad::forward_reachability_searcher frs{};
                auto results = frs.is_reachable(n, query);
                THEN("the query is not satisfied") {
                    REQUIRE_FALSE(results.begin()->satisfied());
                    REQUIRE_FALSE(results.begin()->solution.has_value());
                }
            }
        }
    }
}
