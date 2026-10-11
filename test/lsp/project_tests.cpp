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
#include "temporary_directory.h"
#include "../graphedit/model_builder.h"
#include <lsp/json_rpc.h>
#include <lsp/project.h>
#include <catch2/catch_test_macros.hpp>
#include <fstream>
#include <thread>

using namespace aaltitoad::lsp;
using aaltitoad::test::model_builder;

SCENARIO("framing the messages of the language server protocol", "[lsp]") {
    GIVEN("messages that arrive in pieces") {
        message_reader reader{};
        auto first = frame(R"({"jsonrpc":"2.0","method":"initialized"})");
        auto second = frame(R"({"text":"✓ unicode"})");
        auto stream = first + second;
        THEN("complete messages are read, one at a time") {
            reader.feed(stream.substr(0, 10));
            REQUIRE_FALSE(reader.next().has_value());
            reader.feed(stream.substr(10, first.size()));
            REQUIRE(reader.next() == R"({"jsonrpc":"2.0","method":"initialized"})");
            REQUIRE_FALSE(reader.next().has_value());
            reader.feed(stream.substr(first.size() + 10));
            REQUIRE(reader.next() == R"({"text":"✓ unicode"})");
            REQUIRE_FALSE(reader.next().has_value());
        }
    }
    GIVEN("other headers") {
        message_reader reader{};
        reader.feed("content-type: application/vscode-jsonrpc; charset=utf-8\r\nContent-Length: 2\r\n\r\n{}");
        THEN("they are ignored") {
            REQUIRE(reader.next() == "{}");
        }
    }
    GIVEN("broken frames") {
        THEN("they cannot be read") {
            message_reader no_length{};
            no_length.feed("Content-Type: x\r\n\r\n{}");
            REQUIRE_THROWS_AS(no_length.next(), std::runtime_error);
            message_reader invalid_length{};
            invalid_length.feed("Content-Length: twelve\r\n\r\n{}");
            REQUIRE_THROWS_AS(invalid_length.next(), std::runtime_error);
            message_reader no_colon{};
            no_colon.feed("Content-Length 2\r\n\r\n{}");
            REQUIRE_THROWS_AS(no_colon.next(), std::runtime_error);
        }
    }
}

SCENARIO("matching the excluded files of a project", "[lsp]") {
    glob_matcher matcher{};
    matcher.add("build/**");
    matcher.add("*.tmp.json");
    matcher.add("**/generated/*.json");
    matcher.add("model-?.json");
    matcher.add("{a,b}/x.json");
    matcher.add("[!c]ase.json");
    THEN("paths match as in graphedit") {
        REQUIRE(matcher.matches("build/a/b.json"));
        REQUIRE(matcher.matches("a.tmp.json"));
        REQUIRE_FALSE(matcher.matches("dir/a.tmp.json"));
        REQUIRE(matcher.matches("generated/m.json"));
        REQUIRE(matcher.matches("deep/er/generated/m.json"));
        REQUIRE(matcher.matches("model-1.json"));
        REQUIRE_FALSE(matcher.matches("model-12.json"));
        REQUIRE(matcher.matches("a/x.json"));
        REQUIRE(matcher.matches("b/x.json"));
        REQUIRE_FALSE(matcher.matches("c/x.json"));
        REQUIRE(matcher.matches("base.json"));
        REQUIRE_FALSE(matcher.matches("case.json"));
        REQUIRE_FALSE(matcher.matches("main.json"));
    }
    THEN("special characters of regular expressions are taken literally") {
        glob_matcher literal{};
        literal.add("Light(id).json");
        REQUIRE(literal.matches("Light(id).json"));
        REQUIRE_FALSE(literal.matches("Lightid.json"));
    }
}

SCENARIO("reading the models of a project directory", "[lsp]") {
    aaltitoad::test::temporary_directory_t project{};
    project.write("main.json", model_builder{"main.json", "Main", true}.location("m0", "", "initial").file());
    project.write("lights/light.json", model_builder{"lights/light.json", "Light"}.location("l0", "", "initial").file());
    project.write("project.json", {{"formatVersion", 2}, {"name", "lights"}});
    project.write("lts.json", {{"formatVersion", 2}, {"metadata", {{"graphedit_syntax", "LTS"}}}, {"syntax", nlohmann::json::object()}});
    project.write(".hidden/secret.json", model_builder{".hidden/secret.json", "Secret"}.location("s0", "", "initial").file());
    project.write("build/copy.json", model_builder{"build/copy.json", "Copy"}.location("c0", "", "initial").file());
    project.write("ignored.json", model_builder{"ignored.json", "Ignored"}.location("i0", "", "initial").file());
    project.write("legacy.json", nlohmann::json::parse(R"({"metadata": ["java.util.HashMap", {"graphedit_syntax": "hawk"}], "syntax": {}})"));
    project.write("broken.json", {{"formatVersion", 2}, {"metadata", {{"graphedit_syntax", "NTTA"}}}, {"syntax", {{"vertices", {{"v", 1}}}}}});
    project.write_text("notes.json", "this is not json");
    project.write_text(".graphedit.ignore", "# generated\nignored.json\n");
    project_files files{project.path, {"build/**"}};
    files.scan();
    THEN("the NTTA models are read, except hidden and excluded ones") {
        auto read = files.models();
        REQUIRE(read.size() == 2);
        REQUIRE(read.contains("main.json"));
        REQUIRE(read.contains("lights/light.json"));
        REQUIRE(read.at("lights/light.json").key == "lights/light.json");
    }
    THEN("models that aaltitoad cannot read are reported") {
        auto unreadable = files.unreadable();
        REQUIRE(unreadable.size() == 2);
        REQUIRE(unreadable.at("legacy.json").find("graphedit 1.x") != std::string::npos);
        REQUIRE(unreadable.at("broken.json").find("cannot read the model") != std::string::npos);
    }
    THEN("files are read again when they change, and forgotten when they are gone") {
        // NOTE: the size changes as well, in case the clock of the file system is coarse
        std::this_thread::sleep_for(std::chrono::milliseconds{10});
        project.write("main.json", model_builder{"main.json", "Main(x)", true}.location("m0", "", "initial").location("m1", "").file());
        std::filesystem::remove(project.path / "lights" / "light.json");
        files.scan();
        auto read = files.models();
        REQUIRE(read.size() == 1);
        REQUIRE(read.at("main.json").name == "Main(x)");
    }
}
