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
#include "../graphedit/model_builder.h"
#include <lsp/query.h>
#include <lsp/search.h>
#include <catch2/catch_test_macros.hpp>

using namespace aaltitoad::lsp;
using aaltitoad::test::model_builder;

namespace {
    auto lights() -> aaltitoad::graphedit::network_t {
        return aaltitoad::graphedit::build_network(aaltitoad::test::light_models());
    }

    // A template that counts to a limit (a large state space for a small model)
    auto counter(int limit) -> aaltitoad::graphedit::network_t {
        auto main = model_builder{"main.json", "Main", true}
                .declarations("public int x := 0")
                .location("a", "counting", "initial").location("end", "", "final")
                .edge("loop", "a", "a", "x < " + std::to_string(limit), "x := x + 1");
        return aaltitoad::graphedit::build_network({{"main.json", main.model()}});
    }

    // Why a query cannot be compiled (empty if it can)
    auto error_of(const std::string& query) -> std::string {
        try {
            query_t::compile(query, lights());
        } catch(const std::invalid_argument& e) {
            return e.what();
        }
        return {};
    }

    // Answer a query about the network of lights
    auto answer(const std::string& text, std::size_t max_states = 100000) -> nlohmann::json {
        auto network = lights();
        auto query = query_t::compile(text, network);
        search s{std::move(network), std::move(query), max_states};
        while(!s.run(std::chrono::steady_clock::now() + std::chrono::seconds{1}));
        REQUIRE(s.status() != search::status_t::failed);
        return s.result();
    }

    auto variable(const nlohmann::json& state, const std::string& name) -> nlohmann::json {
        return state.at("variables").at(name).at("value");
    }

    auto active(const nlohmann::json& state, const std::string& instance) -> std::string {
        for(auto& i : state.at("instances"))
            if(i.at("name") == instance)
                return i.at("active").at(0).get<std::string>();
        return {};
    }
}

SCENARIO("compiling queries about networks", "[lsp][verification]") {
    THEN("E F and A G queries over the variables compile, also in the notation of UPPAAL") {
        auto network = lights();
        CHECK(query_t::compile("E F count == 3", network).kind() == query_t::kind_t::reachability);
        CHECK(query_t::compile("A G count <= 3", network).kind() == query_t::kind_t::safety);
        CHECK(query_t::compile("E<> count == 3", network).kind() == query_t::kind_t::reachability);
        CHECK(query_t::compile("  A[] count >= 0", network).kind() == query_t::kind_t::safety);
        CHECK(query_t::compile("E F button.1 && count > 0", network).kind() == query_t::kind_t::reachability);
    }
    THEN("locations are named <instance>@<location>, by their nicknames or their ids, also in parentheses") {
        auto network = lights();
        auto on = query_t::compile("E F Main.lamp(1)@on", network);
        auto lit = query_t::compile("E F Main.lamp(1)@lit", network);
        CHECK_FALSE(on.holds(*network.automata));
        CHECK(query_t::compile("A G (Main.lamp(1)@off || Main.lamp(1)@on) && count >= 0", network).holds(*network.automata));
        CHECK(query_t::compile("E F Main@start", network).holds(*network.automata));
        // the private variable "on" of the instance is a variable, not a location
        CHECK_FALSE(query_t::compile("E F Main.lamp(1).on", network).holds(*network.automata));
    }
    THEN("what aaltitoad-ls does not answer is refused with the reason") {
        CHECK(error_of("count == 3").starts_with("the query is not valid"));
        CHECK(error_of("E G count == 3").starts_with("aaltitoad-ls answers E F p"));
        CHECK(error_of("A F count == 3").starts_with("aaltitoad-ls answers E F p"));
        CHECK(error_of("E F (E F count == 1) && (E F count == 2)").starts_with("aaltitoad-ls does not answer queries in queries"));
        CHECK(error_of("count == 3") == "the query is not valid: syntax error (queries are E F p or A G p, where p is an "
                                         "expression or a location)");
        CHECK(error_of("E F nothing > 1").starts_with("'nothing' is not a variable of the network"));
        CHECK(error_of("E F Main.lamp(3)@on").starts_with("the network has no instance 'Main.lamp(3)'"));
        CHECK(error_of("E F Main.lamp(1)@blue") == "'Main.lamp(1)' has no location 'blue' (a nickname or an id)");
        CHECK(error_of("E F @on").starts_with("a location is named <instance>@<location>"));
        CHECK(error_of("E F count + 1") == "the predicate is not a boolean expression");
        CHECK(error_of("E F 00000000-0000-0000-0000-000000000000").starts_with("no template has the location"));
    }
}

SCENARIO("answering queries about networks", "[lsp][verification]") {
    THEN("a reachable state satisfies E F, with the shortest trace to it") {
        auto result = answer("E F count == 3");
        CHECK(result.at("status") == "satisfied");
        CHECK(result.at("message") == "A state that satisfies the predicate is reached in 3 steps.");
        const auto& steps = result.at("trace").at("steps");
        REQUIRE(steps.size() == 4);
        CHECK_FALSE(steps.at(0).contains("step"));
        CHECK(variable(steps.at(0).at("state"), "count") == 0);
        CHECK(variable(steps.at(3).at("state"), "count") == 3);
        CHECK(steps.at(1).at("step").at("kind") == "step");
        CHECK(steps.at(1).at("step").at("label").get<std::string>().starts_with("Main: start"));
        CHECK(steps.at(1).at("step").at("elements").at(0).at("instance") == "Main");
        CHECK(result.at("statistics").at("states").get<int>() > 3);
    }
    THEN("the changes of the environment are steps of the kind input") {
        auto result = answer("E F Main.lamp(1)@on");
        CHECK(result.at("status") == "satisfied");
        const auto& steps = result.at("trace").at("steps");
        auto input = std::find_if(steps.begin(), steps.end(), [](const nlohmann::json& step) {
            return step.contains("step") && step.at("step").at("kind") == "input";
        });
        REQUIRE(input != steps.end());
        CHECK(input->at("step").at("label").get<std::string>().find("button.1 = true") != std::string::npos);
        CHECK(input->at("step").at("elements").empty());
        CHECK(active(steps.back().at("state"), "Main.lamp(1)") == "lit");
    }
    THEN("a state that violates the predicate violates A G, with the trace to it") {
        auto safe = answer("A G count <= 3");
        CHECK(safe.at("status") == "satisfied");
        CHECK_FALSE(safe.contains("trace"));
        auto unsafe = answer("A G !(Main.lamp(1)@on && Main.lamp(2)@on)");
        CHECK(unsafe.at("status") == "violated");
        const auto& last = unsafe.at("trace").at("steps").back().at("state");
        CHECK(active(last, "Main.lamp(1)") == "lit");
        CHECK(active(last, "Main.lamp(2)") == "lit");
    }
    THEN("E F without a reachable state is violated, and the initial state can decide") {
        auto unreachable = answer("E F count == 5");
        CHECK(unreachable.at("status") == "violated");
        CHECK_FALSE(unreachable.contains("trace"));
        auto initial = answer("E F count == 0");
        CHECK(initial.at("message") == "A state that satisfies the predicate is reached in the initial state.");
        CHECK(initial.at("trace").at("steps").size() == 1);
    }
    THEN("the search is bounded") {
        auto bounded = answer("E F count == 5", 3);
        CHECK(bounded.at("status") == "unknown");
        CHECK(bounded.at("message") == "The search stopped after 3 states (the option maxStates) without an answer.");
    }
    THEN("the search runs in slices") {
        auto network = counter(500);
        auto query = query_t::compile("E F x == 500", network);
        search s{std::move(network), std::move(query), 100000};
        CHECK_FALSE(s.run(std::chrono::steady_clock::now()));
        CHECK(s.status() == search::status_t::searching);
        while(!s.run(std::chrono::steady_clock::now() + std::chrono::seconds{1}));
        CHECK(s.status() == search::status_t::satisfied);
        CHECK(s.result().at("trace").at("steps").size() == 501);
    }
}
