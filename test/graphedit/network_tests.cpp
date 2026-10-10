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
#include "model_builder.h"
#include <parser/graphedit/network.h>
#include <parser/graphedit/problems.h>
#include <catch2/catch_test_macros.hpp>

using namespace aaltitoad::graphedit;
using aaltitoad::test::model_builder;
namespace ids = aaltitoad::graphedit::problems;

namespace {
    // The problems of a model, as "identifier: message"
    auto problems_of(const network_t& network, const std::string& key) -> std::vector<std::string> {
        std::vector<std::string> result{};
        for(auto& problem : network.problems.at(key))
            result.push_back(problem.identifier + ": " + problem.message);
        return result;
    }

    auto identifiers_of(const network_t& network, const std::string& key) -> std::set<std::string> {
        std::set<std::string> result{};
        for(auto& problem : network.problems.at(key))
            result.insert(problem.identifier);
        return result;
    }

    auto with(std::map<std::string, ntta_model_t> models, const model_builder& builder) -> std::map<std::string, ntta_model_t> {
        models[builder.key] = builder.model();
        return models;
    }

    auto light(const std::string& guard, const std::string& update = "on := true") -> model_builder {
        return model_builder{"light.json", "Light(id)"}
                .declarations("private bool on := false")
                .location("off", "off", "initial").location("lit", "on").location("done", "", "final")
                .edge("le1", "off", "lit", guard, update)
                .edge("le2", "lit", "off", "!button.id", "on := false");
    }
}

SCENARIO("building the network of a project", "[graphedit]") {
    GIVEN("a network of lights") {
        auto network = build_network(aaltitoad::test::light_models());
        THEN("aaltitoad finds no problems and builds it") {
            REQUIRE_FALSE(network.has_errors());
            for(auto& [key, problems] : network.problems)
                REQUIRE(problems.empty());
            REQUIRE(network.automata != nullptr);
            REQUIRE(network.automata->components.size() == 3);
        }
        THEN("the instances know their models and what instantiated them") {
            REQUIRE(network.instances.size() == 3);
            REQUIRE(network.instances.at("Main").model == "main.json");
            REQUIRE(network.instances.at("Main").parent.empty());
            REQUIRE(network.instances.at("Main").subcomponent.empty());
            auto& lamp = network.instances.at("Main.lamp(2)");
            REQUIRE(lamp.model == "light.json");
            REQUIRE(lamp.parent == "Main");
            REQUIRE(lamp.subcomponent == "lamp2");
            REQUIRE(lamp.local_identifiers == std::vector<std::string>{"on"});
            REQUIRE(network.instances_of("light.json").size() == 2);
        }
        THEN("the edges of the network know their instances and their edges in the models") {
            auto& edge = network.edges.at("Main.lamp(1).le1");
            REQUIRE(edge.instance == "Main.lamp(1)");
            REQUIRE(edge.model == "light.json");
            REQUIRE(edge.edge.elements == std::vector<std::string>{"le1", "le1.guard", "le1.1", "le1.update", "le1.2"});
            REQUIRE(network.edges.size() == 6);
        }
        THEN("the models without vertices declare the variables of the environment") {
            REQUIRE(network.external_symbols == std::set<std::string>{"button.1", "button.2", "setting"});
            REQUIRE(std::get<int>(network.automata->external_symbols.at("setting")) == 3);
        }
        THEN("the network runs") {
            network.automata->external_symbols["button.2"] = true;
            auto changes = network.automata->tick();
            REQUIRE(changes.size() == 1);
            auto edges = std::set<std::string>{changes[0].edges.begin(), changes[0].edges.end()};
            REQUIRE(edges == std::set<std::string>{"Main.me1", "Main.lamp(2).le1"});
        }
    }
    GIVEN("networks without a main template, with two, or with templates of the same name") {
        auto models = aaltitoad::test::light_models();
        auto no_main = models;
        no_main.at("main.json").is_main = false;
        auto two_mains = models;
        two_mains.at("light.json").is_main = true;
        auto same_names = models;
        same_names.at("light.json").name = "Main";
        THEN("they are reported on the templates, and not built") {
            auto network = build_network(no_main);
            REQUIRE(identifiers_of(network, "main.json") == std::set<std::string>{ids::no_main_template});
            REQUIRE(identifiers_of(network, "light.json") == std::set<std::string>{ids::no_main_template});
            REQUIRE(network.problems.at("io.json").empty());
            REQUIRE(network.automata == nullptr);
            network = build_network(two_mains);
            REQUIRE(identifiers_of(network, "light.json").contains(ids::several_main_templates));
            REQUIRE(network.automata == nullptr);
            network = build_network(same_names);
            REQUIRE(identifiers_of(network, "light.json").contains(ids::duplicate_names));
            REQUIRE(network.automata == nullptr);
        }
    }
    GIVEN("a template without a final location") {
        auto builder = light("button.id");
        builder.graph["vertices"].erase("done");
        auto network = build_network(with(aaltitoad::test::light_models(), builder));
        THEN("it is reported") {
            REQUIRE(problems_of(network, "light.json") ==
                    std::vector<std::string>{std::string{ids::initial_and_final_locations} +
                                             ": aaltitoad runs templates with exactly one initial and one final "
                                             "location, but this one has one initial location and no final location"});
            REQUIRE(network.automata == nullptr);
        }
    }
    GIVEN("expressions that aaltitoad cannot parse") {
        THEN("syntax errors are reported on the nails, with what is wrong") {
            auto network = build_network(with(aaltitoad::test::light_models(), light("button.id &&")));
            REQUIRE(network.problems.at("light.json").size() == 1);
            auto& problem = network.problems.at("light.json").front();
            REQUIRE(problem.identifier == ids::invalid_expression);
            REQUIRE(problem.title == "aaltitoad cannot parse the guard");
            REQUIRE(problem.message.find("syntax error") != std::string::npos);
            REQUIRE(problem.elements == std::vector<std::string>{"le1.guard"});
            REQUIRE(network.automata == nullptr);
        }
        THEN("guards must be expressions, and updates assignments") {
            auto network = build_network(with(aaltitoad::test::light_models(), light("on := true", "on")));
            std::set<std::string> titles{};
            for(auto& problem : network.problems.at("light.json"))
                titles.insert(problem.title);
            REQUIRE(titles == std::set<std::string>{"The guard is not an expression", "The update is not a sequence of assignments"});
        }
        THEN("declarations are checked") {
            auto network = build_network(with(aaltitoad::test::light_models(), light("button.id").declarations("private bool := 1")));
            REQUIRE(identifiers_of(network, "light.json") == std::set<std::string>{ids::invalid_expression});
            auto io = model_builder{"io.json", "io"}.declarations("extern bool := false");
            network = build_network(with(aaltitoad::test::light_models(), io));
            REQUIRE(identifiers_of(network, "io.json") == std::set<std::string>{ids::invalid_expression});
            REQUIRE(network.automata == nullptr);
        }
    }
    GIVEN("a template with an extern declaration") {
        auto network = build_network(with(aaltitoad::test::light_models(), light("button.id").declarations("extern bool on := false")));
        THEN("it is reported, since templates of aaltitoad only declare internal variables") {
            REQUIRE(identifiers_of(network, "light.json") == std::set<std::string>{ids::unsupported_declaration});
            REQUIRE(network.automata == nullptr);
        }
    }
    GIVEN("subcomponents that do not match their templates") {
        auto main = model_builder{"main.json", "Main", true}
                .location("m0", "", "initial").location("m1", "", "final")
                .subcomponent("unknown", "Lamp(id)", "lamp(1)")
                .subcomponent("arguments", "Light(id)", "lamp(1, 2)")
                .subcomponent("nameless", "Light(id)", "")
                .subcomponent("fine", "Light(id)", "lamp(1)");
        auto network = build_network(with(aaltitoad::test::light_models(), main));
        THEN("they are reported on the subcomponents") {
            std::map<std::string, std::string> found{};
            for(auto& problem : network.problems.at("main.json"))
                found[problem.elements.at(0)] = problem.identifier;
            REQUIRE(found == std::map<std::string, std::string>{{"unknown", ids::unknown_template},
                                                                {"arguments", ids::argument_mismatch},
                                                                {"nameless", ids::argument_mismatch}});
            REQUIRE(network.automata == nullptr);
        }
    }
    GIVEN("templates that instantiate each other") {
        auto light_model = light("button.id").subcomponent("inner", "Light(id)", "inner(id)");
        auto network = build_network(with(aaltitoad::test::light_models(), light_model));
        THEN("the circle is reported") {
            REQUIRE(identifiers_of(network, "light.json") == std::set<std::string>{ids::recursive_instantiation});
            REQUIRE(network.automata == nullptr);
        }
    }
    GIVEN("expressions that use undeclared variables") {
        auto network = build_network(with(aaltitoad::test::light_models(), light("button.id && ready", "on := true; level := 1")));
        THEN("they are reported where the variables are used") {
            REQUIRE(network.automata != nullptr);
            REQUIRE(problems_of(network, "light.json") ==
                    std::vector<std::string>{std::string{ids::undeclared_identifier} + ": 'level' is not declared",
                                             std::string{ids::undeclared_identifier} + ": 'ready' is not declared"});
            for(auto& problem : network.problems.at("light.json"))
                REQUIRE(problem.elements.size() == 1);
        }
        THEN("variables that only some instances lack name them") {
            auto io = model_builder{"io.json", "io"}.declarations("extern bool button.1 := false");
            auto missing = build_network(with(aaltitoad::test::light_models(), io));
            // both guards of the template read the button
            REQUIRE(problems_of(missing, "light.json") ==
                    std::vector<std::string>(2, std::string{ids::undeclared_identifier} + ": 'button.2' is not declared (in Main.lamp(2))"));
            REQUIRE(missing.problems.at("light.json")[0].elements != missing.problems.at("light.json")[1].elements);
        }
    }
    GIVEN("declarations that aaltitoad cannot evaluate") {
        auto network = build_network(with(aaltitoad::test::light_models(), light("button.id").declarations("private int on := missing + 1")));
        THEN("the network is not built, and the main template says why") {
            REQUIRE(network.automata == nullptr);
            REQUIRE(identifiers_of(network, "main.json") == std::set<std::string>{ids::build_failed});
            REQUIRE(network.problems.at("main.json").front().message.find("missing") != std::string::npos);
        }
    }
    GIVEN("a broken template that the network does not instantiate") {
        auto unused = model_builder{"unused.json", "Unused"}
                .location("u0", "", "initial").location("u1", "", "final")
                .edge("ue", "u0", "u1", "&&", "");
        auto network = build_network(with(aaltitoad::test::light_models(), unused));
        THEN("its problems are reported, but the network is built") {
            REQUIRE(identifiers_of(network, "unused.json") == std::set<std::string>{ids::invalid_expression});
            REQUIRE(network.automata != nullptr);
        }
    }
}

SCENARIO("expressions of graphedit models", "[graphedit]") {
    THEN("extern and persistent are removed from declarations, but not from strings") {
        auto [declarations, found] = strip_environment_modifiers("extern bool a := false;\npersistent extern string s := \"extern \\\" persistent\";");
        REQUIRE(found);
        REQUIRE(declarations == "bool a := false;\nstring s := \"extern \\\" persistent\";");
        REQUIRE_FALSE(strip_environment_modifiers("public int externals := 1").second);
    }
    THEN("syntax errors are explained") {
        auto parsed = parse_expression("x := ");
        REQUIRE_FALSE(parsed.result.has_value());
        REQUIRE(parsed.error.find("syntax error") != std::string::npos);
        REQUIRE(parse_expression("x > 1 && y").result->expression.has_value());
    }
    THEN("the identifiers of expressions are found") {
        auto parsed = parse_expression("(a.b > 1) && !c || d == \"e\"");
        REQUIRE(identifiers_of(parsed.result->expression.value()) == std::set<std::string>{"a.b", "c", "d"});
    }
}
