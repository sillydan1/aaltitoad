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
#ifndef AALTITOAD_TEST_GRAPHEDIT_MODEL_BUILDER_H
#define AALTITOAD_TEST_GRAPHEDIT_MODEL_BUILDER_H
#include <parser/graphedit/ntta_model.h>
#include <nlohmann/json.hpp>

namespace aaltitoad::test {
    // Writes NTTA models as graphedit sends them to language servers (buffers), for the tests
    class model_builder {
    public:
        model_builder(std::string key, std::string name, bool is_main = false) : key{std::move(key)} {
            metadata = {{"graphedit_syntax", "NTTA"}, {"name", std::move(name)}};
            if(is_main)
                metadata["is_main"] = "true";
        }
        auto declarations(const std::string& text) -> model_builder& {
            graph["declarations"] = text;
            return *this;
        }
        auto location(const std::string& id, const std::string& nickname, const std::string& kind = "normal") -> model_builder& {
            graph["vertices"][id] = {{"type", "location"}, {"position", {{"x", 0}, {"y", 0}}},
                                     {"properties", {{"nickname", nickname}, {"kind", kind}, {"immediate", false}}}};
            return *this;
        }
        auto nail(const std::string& id, const std::string& kind, const std::string& expression) -> model_builder& {
            graph["vertices"][id] = {{"type", "nail"}, {"position", {{"x", 0}, {"y", 0}}},
                                     {"properties", {{"kind", kind}, {"expression", expression}}}};
            return *this;
        }
        auto subcomponent(const std::string& id, const std::string& template_name, const std::string& invocation) -> model_builder& {
            graph["vertices"][id] = {{"type", "subcomponent"}, {"position", {{"x", 0}, {"y", 0}}},
                                     {"properties", {{"template", template_name}, {"invocation", invocation}}}};
            return *this;
        }
        auto connection(const std::string& id, const std::string& source, const std::string& target) -> model_builder& {
            graph["edges"][id] = {{"type", "connection"}, {"source", source}, {"target", target}, {"properties", nlohmann::json::object()}};
            return *this;
        }
        // An edge from a location to a location through a guard nail and an update nail (blank ones are left out)
        auto edge(const std::string& id, const std::string& source, const std::string& target, const std::string& guard,
                  const std::string& update) -> model_builder& {
            auto previous = source;
            auto part = 0;
            auto step = [&](const std::string& next) {
                connection(id + (part == 0 ? "" : "." + std::to_string(part)), previous, next);
                part++;
                previous = next;
            };
            if(!guard.empty()) {
                nail(id + ".guard", "guard", guard);
                step(id + ".guard");
            }
            if(!update.empty()) {
                nail(id + ".update", "update", update);
                step(id + ".update");
            }
            step(target);
            return *this;
        }

        auto buffer() const -> nlohmann::json {
            return {{"key", key}, {"metadata", metadata}, {"graph", graph}};
        }
        auto file() const -> nlohmann::json {
            return {{"formatVersion", 2}, {"metadata", metadata}, {"syntax", graph}};
        }
        auto model() const -> graphedit::ntta_model_t {
            return graphedit::read_buffer(buffer());
        }

        std::string key;
        nlohmann::json metadata{};
        nlohmann::json graph{{"declarations", ""}, {"vertices", nlohmann::json::object()}, {"edges", nlohmann::json::object()}};
    };

    // A network of lights: the main template counts, and has two instances of Light(id), which switch on while their
    // button (a variable of the environment, declared in io.json) is pressed
    inline auto light_models() -> std::map<std::string, graphedit::ntta_model_t> {
        auto main = model_builder{"main.json", "Main", true}
                .declarations("public int count := 0")
                .location("m0", "start", "initial").location("m1", "counting").location("m2", "end", "final")
                .edge("me1", "m0", "m1", "", "count := count + 1")
                .edge("me2", "m1", "m1", "count < 3", "count := count + 1")
                .subcomponent("lamp1", "Light(id)", "lamp(1)")
                .subcomponent("lamp2", "Light(id)", "lamp(2)");
        auto light = model_builder{"light.json", "Light(id)"}
                .declarations("private bool on := false")
                .location("off", "off", "initial").location("lit", "on").location("done", "", "final")
                .edge("le1", "off", "lit", "button.id", "on := true")
                .edge("le2", "lit", "off", "!button.id", "on := false");
        auto io = model_builder{"io.json", "io"}
                .declarations("extern bool button.1 := false;\nextern bool button.2 := false;\npersistent extern int setting := 3;");
        return {{"main.json", main.model()}, {"light.json", light.model()}, {"io.json", io.model()}};
    }
}

#endif //AALTITOAD_TEST_GRAPHEDIT_MODEL_BUILDER_H
