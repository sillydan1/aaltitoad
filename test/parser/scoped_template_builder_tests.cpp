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
#include <catch2/catch_test_macros.hpp>
#include <parser/hawk/scoped_template_builder/scoped_template_builder.h>

namespace {
    using namespace aaltitoad::hawk;

    auto location(const std::string& id) -> model::location_t {
        return {.id=id, .nickname=id, .invariant="", .urgency=model::urgency_t::normal};
    }

    // A light that switches on when its button is pressed, with a private variable
    auto light_template() -> model::tta_template {
        return {.name="Light(id)",
                .declarations="private bool on := false",
                .is_main=false,
                .locations={location("L1")},
                .edges={{.id="e1", .source="L0", .target="L1", .guard="!on && button.id", .update="on := true"},
                        {.id="e2", .source="L1", .target="L0", .guard="on", .update="on := false"}},
                .initial_location=location("L0"),
                .final_location=location("L2"),
                .instances={}};
    }

    auto main_template() -> model::tta_template {
        return {.name="Main",
                .declarations="public int count := 0",
                .is_main=true,
                .locations={},
                .edges={{.id="e1", .source="M0", .target="M1", .guard="", .update="count := count + 1"}},
                .initial_location=location("M0"),
                .final_location=location("M1"),
                .instances={{.id="i1", .tta_template_name="Light(id)", .invocation="lamp(1)"},
                            {.id="i2", .tta_template_name="Light(id)", .invocation="lamp(2)"}}};
    }

    auto build(scoped_template_builder& builder) -> std::unique_ptr<aaltitoad::ntta_t> {
        builder.add_template(main_template()).add_template(light_template());
        builder.add_global_symbols("button.1 := true; button.2 := false");
        return std::unique_ptr<aaltitoad::ntta_t>{builder.build_heap()};
    }

    auto edge_identifiers(const aaltitoad::tta_t& tta) -> std::set<std::string> {
        std::set<std::string> result{};
        for(auto& edge : tta.graph->edges)
            result.insert(edge.first.identifier);
        return result;
    }
}

SCENARIO("building networks of scoped templates", "[scoped_template_builder]") {
    GIVEN("a main template with two instances of a template that has a private variable") {
        scoped_template_builder builder{};
        auto n = build(builder);
        THEN("every instance has its own private variables, and public variables are global") {
            REQUIRE(n->symbols.contains("Main.lamp(1).on"));
            REQUIRE(n->symbols.contains("Main.lamp(2).on"));
            REQUIRE(n->symbols.contains("count"));
            REQUIRE(n->external_symbols.contains("button.1"));
        }
        THEN("guards and updates use the private variables of their instance") {
            auto changes = n->tick();
            REQUIRE(changes.size() == 1);
            n->apply(changes[0]);
            REQUIRE(std::get<bool>(n->symbols.at("Main.lamp(1).on")));
            REQUIRE_FALSE(std::get<bool>(n->symbols.at("Main.lamp(2).on")));
            REQUIRE(std::get<int>(n->symbols.at("count")) == 1);
        }
        THEN("edges are identified by their instance and their id") {
            REQUIRE(edge_identifiers(n->components.at("Main")) == std::set<std::string>{"Main.e1"});
            REQUIRE(edge_identifiers(n->components.at("Main.lamp(1)")) ==
                    std::set<std::string>{"Main.lamp(1).e1", "Main.lamp(1).e2"});
        }
        THEN("state changes tell which edges they take") {
            auto changes = n->tick();
            REQUIRE(changes.size() == 1);
            auto edges = std::set<std::string>{changes[0].edges.begin(), changes[0].edges.end()};
            REQUIRE(edges == std::set<std::string>{"Main.e1", "Main.lamp(1).e1"});
        }
        THEN("the instances are reported with their scopes") {
            const auto& instances = builder.get_instances();
            REQUIRE(instances.size() == 3);
            auto lamp = std::find_if(instances.begin(), instances.end(),
                                     [](const auto& i){ return i.name == "Main.lamp(2)"; });
            REQUIRE(lamp != instances.end());
            REQUIRE(lamp->instance.id == "i2");
            REQUIRE(lamp->instance.tta_template_name == "Light(id)");
            REQUIRE(lamp->parent == "Main");
            REQUIRE(lamp->local_identifiers == std::vector<std::string>{"on"});
            REQUIRE(std::get<int>(lamp->parameters.at("id")) == 2);
            REQUIRE(lamp->edge_identifiers.at("e2") == "Main.lamp(2).e2");
            auto main = std::find_if(instances.begin(), instances.end(),
                                     [](const auto& i){ return i.name == "Main"; });
            REQUIRE(main != instances.end());
            REQUIRE(main->parent.empty());
        }
    }
    GIVEN("a template whose edges have no ids, or the same id") {
        auto light = light_template();
        light.edges[0].id = "";
        light.edges[1].id = "";
        scoped_template_builder builder{};
        builder.add_template(main_template()).add_template(light);
        builder.add_global_symbols("button.1 := true; button.2 := false");
        std::unique_ptr<aaltitoad::ntta_t> n{builder.build_heap()};
        THEN("the edges get identifiers of their own") {
            auto identifiers = edge_identifiers(n->components.at("Main.lamp(1)"));
            REQUIRE(identifiers.size() == 2);
            REQUIRE_FALSE(identifiers.contains("Main.lamp(1)."));
        }
    }
    GIVEN("a guard that is not an expression") {
        auto light = light_template();
        light.edges[0].guard = "on := true";
        scoped_template_builder builder{};
        builder.add_template(main_template()).add_template(light);
        builder.add_global_symbols("button.1 := true; button.2 := false");
        THEN("building the network fails with an error") {
            REQUIRE_THROWS_AS(builder.build_heap(), std::logic_error);
        }
    }
}
