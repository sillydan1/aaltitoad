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
#include "../graphedit/model_builder.h"
#include <debugger/runtime.h>
#include <catch2/catch_test_macros.hpp>

using namespace aaltitoad::debugger;
using aaltitoad::test::model_builder;

namespace {
    auto light_runtime(arguments_t arguments = {}) -> runtime {
        return runtime{aaltitoad::graphedit::build_network(aaltitoad::test::light_models()), arguments};
    }

    // A template that goes from a to b or c (two choices), and back to a
    auto fork_runtime(arguments_t arguments = {}) -> runtime {
        auto main = model_builder{"main.json", "Main", true}
                .declarations("public int visits := 0")
                .location("a", "a", "initial").location("b", "b").location("c", "c").location("end", "", "final")
                .edge("ab", "a", "b", "", "visits := visits + 1")
                .edge("ac", "a", "c", "", "visits := visits + 1")
                .edge("ba", "b", "a", "", "")
                .edge("ca", "c", "a", "", "");
        return runtime{aaltitoad::graphedit::build_network({{"main.json", main.model()}}), arguments};
    }

    auto breakpoint(const std::string& id, const std::string& model, const std::string& element,
                    const std::string& condition = "", const std::string& instance = "", int hit_count = 0) -> breakpoint_t {
        return {id, model, element, instance, condition, hit_count};
    }

    auto variable(const nlohmann::json& state, const std::string& name) -> nlohmann::json {
        return state.at("variables").at(name).at("value");
    }

    auto instance(const nlohmann::json& state, const std::string& name) -> nlohmann::json {
        for(auto& i : state.at("instances"))
            if(i.at("name") == name)
                return i;
        return {};
    }

    auto active(const nlohmann::json& state, const std::string& name) -> std::string {
        return instance(state, name).at("active").at(0).get<std::string>();
    }

    auto far_future() -> std::chrono::steady_clock::time_point {
        return std::chrono::steady_clock::now() + std::chrono::hours{1};
    }
}

SCENARIO("debugging a network of graphedit models", "[debugger]") {
    GIVEN("the network of lights") {
        auto r = light_runtime();
        THEN("its state tells where the instances are, the variables and the steps") {
            auto state = r.state();
            REQUIRE(state.at("id") == "s0");
            REQUIRE(state.at("instances").size() == 3);
            REQUIRE(instance(state, "Main").at("model") == "main.json");
            REQUIRE(active(state, "Main") == "m0");
            REQUIRE(active(state, "Main.lamp(1)") == "off");
            REQUIRE(instance(state, "Main.lamp(1)").at("variables").at("on") ==
                    nlohmann::json{{"value", false}, {"type", "bool"}, {"writable", true}});
            REQUIRE(variable(state, "count") == 0);
            REQUIRE(variable(state, "button.1") == false);
            REQUIRE(variable(state, "setting") == 3);
            REQUIRE_FALSE(state.at("variables").contains("Main.lamp(1).on"));
            REQUIRE(state.at("choices").size() == 1);
            auto choice = state.at("choices").at(0);
            REQUIRE(choice.at("id") == "c0");
            REQUIRE(choice.at("kind") == "step");
            REQUIRE(choice.at("label") == "Main: start → counting");
            REQUIRE(choice.at("elements") == nlohmann::json::array({{{"instance", "Main"}, {"model", "main.json"},
                                                                     {"ids", {"me1", "me1.update", "me1.1"}}}}));
            REQUIRE(state.at("message") == "");
            THEN("reporting the same state again keeps its id") {
                REQUIRE(r.state().at("id") == "s0");
            }
        }
        THEN("a step takes the choice and makes a new state") {
            r.state();
            auto stop = r.step("c0");
            REQUIRE(stop.reason == stop_reason::step);
            REQUIRE(stop.description == "Main: start → counting");
            auto state = r.state();
            REQUIRE(state.at("id") == "s1");
            REQUIRE(active(state, "Main") == "m1");
            REQUIRE(variable(state, "count") == 1);
        }
        THEN("choices that the state does not offer are refused") {
            REQUIRE_THROWS_AS(r.step("c1"), request_error);
            REQUIRE_THROWS_AS(r.step("one"), request_error);
            REQUIRE_THROWS_AS(r.step("c"), request_error);
        }
        THEN("running stops when no edge is enabled") {
            auto stop = r.run(100, far_future());
            REQUIRE(stop.has_value());
            REQUIRE(stop->reason == stop_reason::stuck);
            auto state = r.state();
            REQUIRE(variable(state, "count") == 3);
            REQUIRE(state.at("choices").empty());
            REQUIRE(state.at("message").get<std::string>().find("No edge is enabled") == 0);
            THEN("stepping stops again") {
                REQUIRE(r.step(std::nullopt).reason == stop_reason::stuck);
            }
        }
        THEN("running takes at most the steps that it may") {
            REQUIRE_FALSE(r.run(1, far_future()).has_value());
            REQUIRE(variable(r.state(), "count") == 1);
        }
    }
    GIVEN("breakpoints on locations and edges") {
        auto r = light_runtime();
        auto statuses = r.set_breakpoints({breakpoint("lit", "light.json", "lit", "", "Main.lamp(2)"),
                                           breakpoint("guard", "light.json", "le1.guard")});
        REQUIRE(statuses.size() == 2);
        REQUIRE(statuses[0].verified);
        REQUIRE(statuses[1].verified);
        THEN("they stop the runtime when an instance enters the location or takes the edge") {
            r.set_variables({{"button.2", true}});
            auto stop = r.step(std::nullopt);
            REQUIRE(stop.reason == stop_reason::breakpoint);
            REQUIRE(stop.breakpoints == std::vector<std::string>{"lit", "guard"});
            REQUIRE(stop.description == "Breakpoint at 'on' in Main.lamp(2); Breakpoint at the edge from 'off' to 'on' in Main.lamp(2)");
        }
        THEN("only the instance of a breakpoint counts") {
            r.set_variables({{"button.1", true}});
            auto stop = r.step(std::nullopt);
            REQUIRE(stop.breakpoints == std::vector<std::string>{"guard"});
        }
    }
    GIVEN("breakpoints with conditions and hit counts") {
        auto r = light_runtime();
        THEN("a condition without element stops when it becomes true") {
            REQUIRE(r.set_breakpoints({breakpoint("two", "", "", "count == 2")})[0].verified);
            REQUIRE(r.step(std::nullopt).reason == stop_reason::step);
            auto stop = r.step(std::nullopt);
            REQUIRE(stop.reason == stop_reason::breakpoint);
            REQUIRE(stop.description == "Breakpoint: count == 2");
            THEN("but not while it holds") {
                r.set_breakpoints({breakpoint("big", "", "", "count >= 2")});
                REQUIRE(r.step(std::nullopt).reason == stop_reason::step);
            }
        }
        THEN("conditions of breakpoints on elements use the variables of their instance") {
            r.set_variables({{"button.1", true}, {"button.2", true}});
            REQUIRE(r.set_breakpoints({breakpoint("lit", "light.json", "lit", "on && count == 0")})[0].verified);
            auto stop = r.step(std::nullopt);
            REQUIRE(stop.reason == stop_reason::step); // count is 1 after the step
            r.set_breakpoints({breakpoint("off", "light.json", "off", "!on")});
            r.set_variables({{"button.1", false}});
            stop = r.step(std::nullopt);
            REQUIRE(stop.reason == stop_reason::breakpoint);
            REQUIRE(stop.description == "Breakpoint at 'off' in Main.lamp(1)");
        }
        THEN("hit counts skip the first hits") {
            r.set_breakpoints({breakpoint("counting", "main.json", "m1", "", "", 2)});
            REQUIRE(r.step(std::nullopt).reason == stop_reason::step);
            auto stop = r.step(std::nullopt);
            REQUIRE(stop.reason == stop_reason::breakpoint);
            THEN("and so do the hits from before a breakpoint changed") {
                r.set_breakpoints({breakpoint("counting", "main.json", "m1", "", "", 3)});
                REQUIRE(r.step(std::nullopt).reason == stop_reason::step);
            }
        }
        THEN("breakpoints that stay the same keep their hits") {
            r.set_breakpoints({breakpoint("counting", "main.json", "m1", "", "", 2)});
            r.step(std::nullopt);
            r.set_breakpoints({breakpoint("counting", "main.json", "m1", "", "", 2), breakpoint("other", "", "", "count > 5")});
            REQUIRE(r.step(std::nullopt).reason == stop_reason::breakpoint);
        }
    }
    GIVEN("breakpoints that the runtime cannot use") {
        auto r = light_runtime();
        auto statuses = r.set_breakpoints({breakpoint("model", "nowhere.json", "x"),
                                           breakpoint("element", "light.json", "ghost"),
                                           breakpoint("subcomponent", "main.json", "lamp1"),
                                           breakpoint("syntax", "", "", "count >"),
                                           breakpoint("undeclared", "", "", "nothing > 1"),
                                           breakpoint("number", "", "", "count + 1"),
                                           breakpoint("instance", "light.json", "lit", "", "Main.lamp(3)"),
                                           breakpoint("wrong instance", "light.json", "lit", "", "Main")});
        THEN("they are not verified, and say why") {
            std::map<std::string, std::string> messages{};
            for(auto& status : statuses) {
                REQUIRE_FALSE(status.verified);
                messages[status.id] = status.message;
            }
            REQUIRE(messages.at("model") == "'nowhere.json' is not a model of the network");
            REQUIRE(messages.at("element") == "the element is not a location, and no edge of the template passes through it");
            REQUIRE(messages.at("subcomponent").find("subcomponents are not supported") != std::string::npos);
            REQUIRE(messages.at("syntax").find("the condition is not valid: syntax error") == 0);
            REQUIRE(messages.at("undeclared") == "'nothing' is not declared");
            REQUIRE(messages.at("number") == "the condition is not a boolean expression");
            REQUIRE(messages.at("instance") == "the network has no instance 'Main.lamp(3)'");
            REQUIRE(messages.at("wrong instance") == "'Main' is not an instance of 'light.json'");
        }
        THEN("they are never hit") {
            REQUIRE(r.run(100, far_future())->reason == stop_reason::stuck);
        }
    }
    GIVEN("breakpoints in json") {
        THEN("they are read") {
            auto b = breakpoint_from_json({{"id", "b1"}, {"element", {{"model", "a.json"}, {"id", "v"}}}, {"instance", "Main"},
                                           {"condition", "x > 1"}, {"hitCount", 3}});
            REQUIRE(b == breakpoint("b1", "a.json", "v", "x > 1", "Main", 3));
            REQUIRE(breakpoint_from_json({{"id", "b2"}, {"condition", "x"}}).element.empty());
        }
        THEN("broken ones are refused") {
            REQUIRE_THROWS_AS(breakpoint_from_json({{"element", {{"model", "a.json"}, {"id", "v"}}}}), request_error);
            REQUIRE_THROWS_AS(breakpoint_from_json({{"id", "b"}}), request_error);
            REQUIRE_THROWS_AS(breakpoint_from_json({{"id", "b"}, {"condition", "x"}, {"hitCount", -1}}), request_error);
            REQUIRE_THROWS_AS(breakpoint_from_json({{"id", "b"}, {"element", {{"model", "a.json"}}}}), request_error);
            REQUIRE_THROWS_AS(breakpoint_from_json(42), request_error);
        }
    }
    GIVEN("variables to change") {
        auto r = light_runtime();
        THEN("global and instance variables change") {
            r.set_variables({{"count", 2}, {"button.1", true}, {"Main.lamp(1):on", true}});
            auto state = r.state();
            REQUIRE(variable(state, "count") == 2);
            REQUIRE(variable(state, "button.1") == true);
            REQUIRE(instance(state, "Main.lamp(1)").at("variables").at("on").at("value") == true);
            REQUIRE(instance(state, "Main.lamp(2)").at("variables").at("on").at("value") == false);
        }
        THEN("nothing changes if one of them cannot") {
            REQUIRE_THROWS_AS(r.set_variables({{"count", 7}, {"nothing", 1}}), request_error);
            REQUIRE_THROWS_AS(r.set_variables({{"count", true}}), request_error);
            REQUIRE_THROWS_AS(r.set_variables({{"count", 1.5}}), request_error);
            REQUIRE_THROWS_AS(r.set_variables({{"Main.lamp(1).on", true}}), request_error);
            REQUIRE_THROWS_AS(r.set_variables({{"Main.lamp(9):on", true}}), request_error);
            REQUIRE_THROWS_AS(r.set_variables({{"Main.lamp(1):off", true}}), request_error);
            REQUIRE_THROWS_AS(r.set_variables(nlohmann::json::array()), request_error);
            REQUIRE(variable(r.state(), "count") == 0);
        }
        THEN("the steps follow the new values") {
            r.set_variables({{"button.1", true}});
            auto choice = r.state().at("choices").at(0);
            REQUIRE(choice.at("label") == "Main: start → counting, Main.lamp(1): off → on");
            REQUIRE(choice.at("elements").size() == 2);
        }
    }
    GIVEN("instances to put into other locations") {
        auto r = light_runtime();
        THEN("they go there") {
            r.set_active("Main.lamp(1)", {"lit"});
            REQUIRE(active(r.state(), "Main.lamp(1)") == "lit");
        }
        THEN("locations and instances that do not exist are refused") {
            REQUIRE_THROWS_AS(r.set_active("Main.lamp(1)", {"m1"}), request_error);
            REQUIRE_THROWS_AS(r.set_active("Main.lamp(1)", {"off", "lit"}), request_error);
            REQUIRE_THROWS_AS(r.set_active("Main.lamp(7)", {"off"}), request_error);
        }
    }
    GIVEN("states that were reported") {
        auto r = light_runtime({.policy=policy_t::first, .seed={}, .delay=0, .history=2});
        auto first = r.state().at("id").get<std::string>();
        r.step(std::nullopt);
        auto second = r.state().at("id").get<std::string>();
        THEN("they can be restored, and keep their ids") {
            r.restore(first);
            auto state = r.state();
            REQUIRE(state.at("id") == first);
            REQUIRE(variable(state, "count") == 0);
            REQUIRE(active(state, "Main") == "m0");
            r.restore(second);
            REQUIRE(variable(r.state(), "count") == 1);
        }
        THEN("live states are not kept, unless they are reported again") {
            auto live = r.state(true).at("id").get<std::string>();
            REQUIRE(live == second);
            r.step(std::nullopt);
            auto running = r.state(true).at("id").get<std::string>();
            REQUIRE_THROWS_AS(r.restore(running), request_error);
            REQUIRE(r.state().at("id") == running);
            REQUIRE_NOTHROW(r.restore(running));
        }
        THEN("only as many as the history holds") {
            r.step(std::nullopt);
            r.state();
            REQUIRE_THROWS_AS(r.restore(first), request_error);
            REQUIRE_NOTHROW(r.restore(second));
            REQUIRE_THROWS_AS(r.restore("s99"), request_error);
        }
    }
    GIVEN("a state with several choices") {
        auto r = fork_runtime();
        THEN("every choice is offered and can be taken") {
            auto state = r.state();
            REQUIRE(state.at("choices").size() == 2);
            std::set<std::string> labels{};
            for(auto& choice : state.at("choices"))
                labels.insert(choice.at("label").get<std::string>());
            REQUIRE(labels == std::set<std::string>{"Main: a → b", "Main: a → c"});
            auto second = state.at("choices").at(1);
            r.step(second.at("id").get<std::string>());
            auto target = second.at("label").get<std::string>().back() == 'b' ? "b" : "c";
            REQUIRE(active(r.state(), "Main") == target);
        }
        THEN("random runs can be repeated with a seed") {
            auto walk = [](unsigned int seed) {
                auto r = fork_runtime({.policy=policy_t::random, .seed=seed, .delay=0, .history=10});
                std::string visited{};
                for(int i = 0; i < 20; i++) {
                    r.step(std::nullopt);
                    visited += active(r.state(), "Main");
                }
                return visited;
            };
            REQUIRE(walk(7) == walk(7));
            REQUIRE(walk(7).find('b') != std::string::npos);
            REQUIRE(walk(7).find('c') != std::string::npos);
        }
    }
    GIVEN("steps that take many edges") {
        // Main goes from a to b or c, and four timers tick along in every step
        auto main = model_builder{"main.json", "Main", true}
                .location("a", "a", "initial").location("b", "b").location("c", "c").location("end", "", "final")
                .edge("ab", "a", "b", "", "").edge("ac", "a", "c", "", "").edge("ba", "b", "a", "", "").edge("ca", "c", "a", "", "")
                .subcomponent("t1", "Timer(n)", "t(1)").subcomponent("t2", "Timer(n)", "t(2)")
                .subcomponent("t3", "Timer(n)", "t(3)").subcomponent("t4", "Timer(n)", "t(4)");
        auto timer = model_builder{"timer.json", "Timer(n)"}
                .location("x", "x", "initial").location("y", "y").location("done", "", "final")
                .edge("xy", "x", "y", "", "").edge("yx", "y", "x", "", "");
        runtime r{aaltitoad::graphedit::build_network({{"main.json", main.model()}, {"timer.json", timer.model()}}), {}};
        THEN("the labels of the choices leave out the edges that every choice takes") {
            auto state = r.state();
            REQUIRE(state.at("choices").size() == 2);
            REQUIRE(state.at("choices").at(0).at("label").get<std::string>().find(" (and the 4 edges that every step takes)") != std::string::npos);
            REQUIRE(state.at("choices").at(0).at("label").get<std::string>().find("Main: a \u2192 ") == 0);
            REQUIRE(state.at("choices").at(0).at("elements").size() == 5);
        }
        THEN("labels name a few edges, and count the others") {
            auto stop = r.step("c0");
            REQUIRE(stop.description == "Main: a \u2192 b, Main.t(1): x \u2192 y, Main.t(2): x \u2192 y and 2 more");
            REQUIRE(r.state().at("choices").at(0).at("label") == "Main: b \u2192 a, Main.t(1): y \u2192 x, Main.t(2): y \u2192 x and 2 more");
        }
    }
    GIVEN("choices that lead to locations without nicknames") {
        auto main = model_builder{"main.json", "Main", true}
                .declarations("public int n := 0")
                .location("a", "a", "initial").location("x", "").location("y", "").location("z", "").location("end", "", "final")
                .edge("ax", "a", "x", "n >= 0", "").edge("ay", "a", "y", "n < 100", "")
                .edge("az1", "a", "z", "", "n := 1").edge("az2", "a", "z", "", "n := 1");
        runtime r{aaltitoad::graphedit::build_network({{"main.json", main.model()}}), {}};
        THEN("their labels name the guards or updates of their edges, or else their ids") {
            std::set<std::string> labels{};
            auto state = r.state();
            INFO(state.at("message").get<std::string>());
            for(auto& choice : state.at("choices"))
                labels.insert(choice.at("label").get<std::string>());
            REQUIRE(labels.size() == 4);
            REQUIRE(labels.contains("Main: a \u2192 location if n >= 0"));
            REQUIRE(labels.contains("Main: a \u2192 location if n < 100"));
            auto by_id = std::count_if(labels.begin(), labels.end(), [](const std::string& l) {
                return l.starts_with("Main: a \u2192 location do n := 1 (c");
            });
            REQUIRE(by_id == 2);
        }
    }
    GIVEN("a guard that is not boolean") {
        auto main = model_builder{"main.json", "Main", true}
                .declarations("public int count := 0")
                .location("a", "a", "initial").location("b", "b").location("end", "", "final")
                .edge("ab", "a", "b", "count + 1", "");
        runtime r{aaltitoad::graphedit::build_network({{"main.json", main.model()}}), {}};
        THEN("the steps cannot be computed, and the runtime says why") {
            auto state = r.state();
            REQUIRE(state.at("choices").empty());
            REQUIRE(state.at("message").get<std::string>().find("aaltitoad cannot compute the steps") == 0);
            auto stop = r.step(std::nullopt);
            REQUIRE(stop.reason == stop_reason::error);
            REQUIRE(stop.description.find("wrong type") != std::string::npos);
        }
    }
    GIVEN("arguments of a configuration") {
        THEN("they are read") {
            auto arguments = arguments_from_json({{"policy", "random"}, {"seed", 3}, {"delay", 250}, {"history", 50}});
            REQUIRE(arguments.policy == policy_t::random);
            REQUIRE(arguments.seed == 3u);
            REQUIRE(arguments.delay == 250);
            REQUIRE(arguments.history == 50u);
            REQUIRE(arguments_from_json(nullptr).policy == policy_t::first);
        }
        THEN("invalid ones are refused") {
            REQUIRE_THROWS_AS(arguments_from_json({{"policy", "best"}}), request_error);
            REQUIRE_THROWS_AS(arguments_from_json({{"delay", -1}}), request_error);
            REQUIRE_THROWS_AS(arguments_from_json({{"history", 0}}), request_error);
            REQUIRE_THROWS_AS(arguments_from_json(nlohmann::json::array()), request_error);
        }
    }
    GIVEN("a network that cannot be built") {
        auto models = aaltitoad::test::light_models();
        models.at("main.json").is_main = false;
        THEN("there is no runtime for it") {
            REQUIRE_THROWS_AS((runtime{aaltitoad::graphedit::build_network(models), {}}), std::invalid_argument);
        }
    }
}
