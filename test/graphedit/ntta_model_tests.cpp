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
#include "model_builder.h"
#include <parser/graphedit/problems.h>
#include <catch2/catch_test_macros.hpp>

using namespace aaltitoad::graphedit;
using aaltitoad::test::model_builder;

namespace {
    auto identifiers(const std::vector<problem_t>& problems) -> std::vector<std::string> {
        std::vector<std::string> result{};
        for(auto& problem : problems)
            result.push_back(problem.identifier);
        return result;
    }
}

SCENARIO("reading NTTA models of graphedit", "[graphedit]") {
    GIVEN("a template with locations, nails and a subcomponent") {
        auto builder = model_builder{"lights/light.json", "Light(id)", true}
                .declarations("public bool on := false")
                .location("l0", "off", "initial").location("l1", "on").location("l2", "", "final")
                .nail("n1", "guard", "button.id").nail("n2", "update", "on := true")
                .subcomponent("s1", "Timer(t)", "timer(1)");
        WHEN("reading it") {
            auto model = builder.model();
            THEN("its metadata, declarations and vertices are read") {
                REQUIRE(model.key == "lights/light.json");
                REQUIRE(model.name == "Light(id)");
                REQUIRE(model.is_main);
                REQUIRE(model.declarations == "public bool on := false");
                REQUIRE(model.is_template());
                REQUIRE(model.locations.size() == 3);
                REQUIRE(model.find_location("l0")->kind == location_kind_t::initial);
                REQUIRE(model.find_location("l0")->nickname == "off");
                REQUIRE(model.find_location("l2")->kind == location_kind_t::final);
                REQUIRE(model.find_nail("n1")->kind == nail_kind_t::guard);
                REQUIRE(model.find_nail("n2")->expression == "on := true");
                REQUIRE(model.find_subcomponent("s1")->template_name == "Timer(t)");
                REQUIRE(model.find_subcomponent("s1")->invocation == "timer(1)");
            }
        }
        WHEN("properties are missing") {
            builder.graph["vertices"]["l1"]["properties"] = nlohmann::json::object();
            builder.graph["vertices"]["n1"].erase("properties");
            builder.metadata["is_main"] = " True ";
            auto model = builder.model();
            THEN("they have their default values") {
                REQUIRE(model.find_location("l1")->kind == location_kind_t::normal);
                REQUIRE(model.find_location("l1")->nickname.empty());
                REQUIRE_FALSE(model.find_location("l1")->immediate);
                REQUIRE(model.find_nail("n1")->kind == nail_kind_t::comment);
                REQUIRE(model.find_nail("n1")->expression.empty());
                REQUIRE(model.is_main);
            }
        }
    }
    GIVEN("an edge through guard, comment and update nails") {
        auto model = model_builder{"a.json", "A"}
                .location("l0", "", "initial").location("l1", "")
                .nail("g1", "guard", "x > 1").nail("c", "comment", "why").nail("g2", "guard", " y ")
                .nail("u1", "update", "x := 0").nail("blank", "update", "  ").nail("u2", "update", "y := false")
                .connection("c1", "l0", "g1").connection("c2", "g1", "c").connection("c3", "c", "g2")
                .connection("c4", "g2", "u1").connection("c5", "u1", "blank").connection("c6", "blank", "u2")
                .connection("c7", "u2", "l1")
                .model();
        THEN("the edge leads from location to location, with the guards conjoined and the updates in order") {
            REQUIRE(model.problems.empty());
            REQUIRE(model.edges.size() == 1);
            const auto& edge = model.edges.front();
            REQUIRE(edge.id == "c1");
            REQUIRE(edge.source == "l0");
            REQUIRE(edge.target == "l1");
            REQUIRE(edge.guard == "(x > 1) && (y)");
            REQUIRE(edge.update == "x := 0; y := false");
            REQUIRE(edge.elements == std::vector<std::string>{"c1", "g1", "c2", "c", "c3", "g2", "c4", "u1", "c5", "blank", "c6", "u2", "c7"});
            REQUIRE(edge.guard_nails == std::vector<std::string>{"g1", "g2"});
            REQUIRE(edge.update_nails == std::vector<std::string>{"u1", "u2"});
            REQUIRE(model.edges_through("u2").size() == 1);
            REQUIRE(model.edges_through("l0").empty());
        }
    }
    GIVEN("edges that share their nails, a single guard and a self loop") {
        auto model = model_builder{"a.json", "A"}
                .location("l0", "", "initial").location("l1", "").location("l2", "")
                .nail("g", "guard", "ready").nail("u", "update", "x := 1")
                .connection("from0", "l0", "g").connection("from1", "l1", "g").connection("c2", "g", "u")
                .connection("c3", "u", "l2").connection("loop", "l2", "l2")
                .model();
        THEN("every edge starts with its own connection") {
            REQUIRE(model.problems.empty());
            REQUIRE(model.edges.size() == 3);
            REQUIRE(model.edges_through("u").size() == 2);
            for(auto& edge : model.edges) {
                if(edge.id == "loop") {
                    REQUIRE(edge.source == "l2");
                    REQUIRE(edge.target == "l2");
                    REQUIRE(edge.elements == std::vector<std::string>{"loop"});
                    REQUIRE(edge.guard.empty());
                } else {
                    REQUIRE(edge.guard == "ready");
                    REQUIRE(edge.target == "l2");
                }
            }
        }
    }
    GIVEN("edges that do not arrive at a location") {
        auto model = model_builder{"a.json", "A"}
                .location("l0", "", "initial").location("l1", "")
                .nail("nowhere", "guard", "a").nail("fork", "guard", "b").nail("ring1", "comment", "").nail("ring2", "comment", "")
                .connection("c1", "l0", "nowhere")
                .connection("c2", "l0", "fork").connection("c3", "fork", "l0").connection("c4", "fork", "l1")
                .connection("c5", "l1", "ring1").connection("c6", "ring1", "ring2").connection("c7", "ring2", "ring1")
                .connection("c8", "l1", "ghost")
                .connection("c9", "l0", "nowhere")
                .model();
        THEN("they are left out, and every problem is reported once") {
            REQUIRE(model.edges.empty());
            REQUIRE(identifiers(model.problems) == std::vector<std::string>(4, aaltitoad::graphedit::problems::broken_edge));
            REQUIRE(model.problems[0].message == "The nail leads nowhere");
            REQUIRE(model.problems[0].elements == std::vector<std::string>{"nowhere"});
        }
    }
    GIVEN("connections into and out of a subcomponent") {
        auto model = model_builder{"a.json", "A"}
                .location("l0", "", "initial").location("l1", "", "final")
                .subcomponent("s", "B", "b")
                .connection("in", "l0", "s").connection("out", "s", "l1")
                .model();
        THEN("they are reported, since aaltitoad does not run them") {
            REQUIRE(model.edges.empty());
            REQUIRE(identifiers(model.problems) == std::vector<std::string>{aaltitoad::graphedit::problems::subcomponent_connection,
                                                                            aaltitoad::graphedit::problems::subcomponent_connection});
        }
    }
    GIVEN("a model without vertices") {
        auto model = model_builder{"io.json", "io"}.declarations("extern bool button := false").model();
        THEN("it only declares symbols") {
            REQUIRE_FALSE(model.is_template());
            REQUIRE(model.declarations == "extern bool button := false");
        }
    }
    GIVEN("model files") {
        auto builder = model_builder{"a.json", "A"}.location("l0", "", "initial");
        THEN("NTTA models of the current format are read") {
            auto model = read_model_file("a.json", builder.file());
            REQUIRE(model.has_value());
            REQUIRE(model->locations.size() == 1);
        }
        THEN("models of other syntaxes, other formats and other files are not") {
            auto other = builder.file();
            other["metadata"]["graphedit_syntax"] = "LTS";
            REQUIRE_FALSE(read_model_file("a.json", other).has_value());
            auto old = builder.file();
            old["formatVersion"] = 1;
            REQUIRE_FALSE(read_model_file("a.json", old).has_value());
            REQUIRE_FALSE(read_model_file("project.json", {{"name", "project"}, {"exclude", nlohmann::json::array()}}).has_value());
            REQUIRE_FALSE(is_legacy_model_file(builder.file()));
        }
        THEN("models of graphedit 1.x are recognized") {
            auto legacy = nlohmann::json::parse(R"({"metadata": ["java.util.HashMap", {"graphedit_syntax": "hawk", "name": "Fischer"}],
                                                    "syntax": {"declarations": "", "vertices": ["java.util.HashMap", {}]}})");
            REQUIRE(is_legacy_model_file(legacy));
            REQUIRE_FALSE(read_model_file("a.json", legacy).has_value());
        }
        THEN("broken json is refused") {
            auto broken = builder.file();
            broken["syntax"]["vertices"]["l0"] = 42;
            REQUIRE_THROWS_AS(read_model_file("a.json", broken), std::invalid_argument);
            REQUIRE_THROWS_AS(read_buffer(nlohmann::json{{"graph", nlohmann::json::object()}}), std::invalid_argument);
        }
    }
}
