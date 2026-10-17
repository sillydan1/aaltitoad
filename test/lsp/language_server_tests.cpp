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
#include <lsp/language_server.h>
#include <parser/graphedit/problems.h>
#include <catch2/catch_test_macros.hpp>
#include <thread>

using namespace aaltitoad::lsp;
using aaltitoad::test::model_builder;

namespace {
    // A client of the language server, which collects everything the server sends
    struct client_t {
        std::vector<nlohmann::json> messages{};
        language_server server{[this](const std::string& message) { messages.push_back(nlohmann::json::parse(message)); }, "test"};
        int next_id{1};

        // Send a request and return the response (the first message that the server sends)
        auto request(const std::string& method, const nlohmann::json& params = nlohmann::json::object()) -> nlohmann::json {
            auto id = next_id++;
            auto before = messages.size();
            server.handle(nlohmann::json{{"jsonrpc", "2.0"}, {"id", id}, {"method", method}, {"params", params}}.dump());
            REQUIRE(messages.size() > before);
            REQUIRE(messages.at(before).at("id") == id);
            return messages.at(before);
        }
        auto result(const std::string& method, const nlohmann::json& params = nlohmann::json::object()) -> nlohmann::json {
            auto response = request(method, params);
            INFO(response.dump());
            REQUIRE(response.contains("result"));
            return response.at("result");
        }
        auto error_code(const std::string& method, const nlohmann::json& params = nlohmann::json::object()) -> int {
            auto response = request(method, params);
            INFO(response.dump());
            REQUIRE(response.contains("error"));
            return response.at("error").at("code").get<int>();
        }
        void notify(const std::string& method, const nlohmann::json& params = nlohmann::json::object()) {
            server.handle(nlohmann::json{{"jsonrpc", "2.0"}, {"method", method}, {"params", params}}.dump());
        }
        // The params of the notifications of a method, from the message with the index on
        auto notifications(const std::string& method, std::size_t since = 0) const -> std::vector<nlohmann::json> {
            std::vector<nlohmann::json> result{};
            for(auto i = since; i < messages.size(); i++)
                if(messages[i].value("method", "") == method)
                    result.push_back(messages[i].at("params"));
            return result;
        }
        // Wait for the work that is due, and do it
        void work() {
            if(auto due = server.next_work())
                std::this_thread::sleep_until(due.value());
            server.work();
        }
        void initialize(const std::filesystem::path& project = {}) {
            result("initialize", {{"processId", 1}, {"project", {{"path", project.string()}, {"excludeFiles", nlohmann::json::array()}}},
                                  {"capabilities", nlohmann::json::object()}});
            notify("initialized");
        }
        // The diagnostics that were published last for every model, by key
        auto diagnostics() const -> std::map<std::string, std::vector<std::string>> {
            std::map<std::string, std::vector<std::string>> result{};
            for(auto& params : notifications("graph/publishDiagnostics")) {
                std::vector<std::string> identifiers{};
                for(auto& diagnostic : params.at("diagnostics"))
                    identifiers.push_back(diagnostic.at("lintIdentifier").get<std::string>());
                result[params.at("key").get<std::string>()] = identifiers;
            }
            return result;
        }
    };

    auto light_buffers() -> nlohmann::json {
        auto main = model_builder{"main.json", "Main", true}
                .declarations("public int count := 0")
                .location("m0", "start", "initial").location("m1", "counting").location("m2", "end", "final")
                .edge("me1", "m0", "m1", "", "count := count + 1")
                .edge("me2", "m1", "m1", "count < 3", "count := count + 1");
        return nlohmann::json::array({main.buffer()});
    }

    // A network that never stops by itself: it counts forever
    auto endless_buffers() -> nlohmann::json {
        auto main = model_builder{"main.json", "Main", true}
                .declarations("public int count := 0")
                .location("m0", "start", "initial").location("m1", "counting").location("m2", "end", "final")
                .edge("me1", "m0", "m1", "", "")
                .edge("me2", "m1", "m1", "", "count := count + 1");
        return nlohmann::json::array({main.buffer()});
    }

    auto launch_params(const nlohmann::json& models, bool stop_on_entry, const nlohmann::json& arguments = nlohmann::json::object(),
                       const nlohmann::json& breakpoints = nlohmann::json::array()) -> nlohmann::json {
        return {{"configuration", {{"name", "NTTA"}, {"arguments", arguments}}}, {"models", models},
                {"breakpoints", breakpoints}, {"stopOnEntry", stop_on_entry}};
    }
}

SCENARIO("the lifecycle of the language server", "[lsp]") {
    client_t client{};
    THEN("initialize tells the language and what the server can do") {
        auto result = client.result("initialize", {{"processId", 1}, {"capabilities", nlohmann::json::object()}});
        REQUIRE(result.at("serverInfo") == nlohmann::json{{"name", "aaltitoad-ls"}, {"version", "test"}});
        REQUIRE(result.at("language") == "NTTA");
        REQUIRE(result.at("capabilities").at("sync") == "full");
        REQUIRE(result.at("capabilities").at("diagnostics") == true);
        auto debug = result.at("capabilities").at("debug");
        REQUIRE(debug.at("launch") == true);
        REQUIRE(debug.at("attach") == false);
        for(auto capability : {"chooseSteps", "history", "setVariables", "setActive", "breakpoints", "conditionBreakpoints", "hitCounts", "liveState"})
            REQUIRE(debug.at(capability) == true);
        THEN("it cannot be initialized twice") {
            REQUIRE(client.error_code("initialize") == invalid_request);
        }
    }
    THEN("requests before initialize are refused") {
        REQUIRE(client.error_code("shutdown") == server_not_initialized);
    }
    THEN("unknown methods are refused") {
        client.initialize();
        REQUIRE(client.error_code("graph/format") == method_not_found);
        REQUIRE(client.error_code("debug/stepBack") == method_not_found);
    }
    THEN("broken messages are answered with errors") {
        client.server.handle("{not json");
        REQUIRE(client.messages.back().at("error").at("code") == parse_error);
        REQUIRE(client.messages.back().at("id").is_null());
        client.server.handle(R"({"jsonrpc": "2.0", "id": 9})");
        REQUIRE(client.messages.back().at("error").at("code") == invalid_request);
        REQUIRE(client.messages.back().at("id") == 9);
        auto count = client.messages.size();
        client.server.handle(R"({"jsonrpc": "2.0", "id": 9, "result": null})");
        REQUIRE(client.messages.size() == count);
    }
    THEN("the server exits after shutdown and exit") {
        client.initialize();
        REQUIRE(client.result("shutdown").is_null());
        REQUIRE(client.error_code("debug/state") == invalid_request);
        REQUIRE_FALSE(client.server.should_exit());
        client.notify("exit");
        REQUIRE(client.server.should_exit());
        REQUIRE(client.server.exit_code() == 0);
    }
    THEN("exiting without shutdown is an error") {
        client.initialize();
        client.notify("exit");
        REQUIRE(client.server.should_exit());
        REQUIRE(client.server.exit_code() == 1);
    }
}

SCENARIO("the language server publishes the problems of a project", "[lsp]") {
    aaltitoad::test::temporary_directory_t project{};
    auto broken = model_builder{"light.json", "Light(id)"}
            .declarations("private bool on := false")
            .location("off", "off", "initial").location("lit", "on").location("done", "", "final")
            .edge("le1", "off", "lit", "button.id &&", "on := true");
    auto fixed = model_builder{"light.json", "Light(id)"}
            .declarations("private bool on := false")
            .location("off", "off", "initial").location("lit", "on").location("done", "", "final")
            .edge("le1", "off", "lit", "button.id", "on := true");
    auto main = model_builder{"main.json", "Main", true}
            .location("m0", "", "initial").location("m1", "", "final")
            .subcomponent("lamp1", "Light(id)", "lamp(1)");
    auto io = model_builder{"io.json", "io"}.declarations("extern bool button.1 := false");
    project.write("main.json", main.file());
    project.write("light.json", broken.file());
    project.write("io.json", io.file());
    project.write("old.json", nlohmann::json::parse(R"({"metadata": ["java.util.HashMap", {"graphedit_syntax": "hawk"}], "syntax": {}})"));
    client_t client{};
    client.initialize(project.path);
    client.work();
    THEN("the problems of the model files are published after initialize") {
        auto diagnostics = client.diagnostics();
        REQUIRE(diagnostics.at("light.json") == std::vector<std::string>{aaltitoad::graphedit::problems::invalid_expression});
        REQUIRE(diagnostics.at("old.json") == std::vector<std::string>{aaltitoad::graphedit::problems::unreadable_model});
        REQUIRE_FALSE(diagnostics.contains("main.json"));
        auto published = client.notifications("graph/publishDiagnostics");
        auto light = std::find_if(published.begin(), published.end(), [](auto& p){ return p.at("key") == "light.json"; });
        auto diagnostic = light->at("diagnostics").at(0);
        REQUIRE(diagnostic.at("severity") == "error");
        REQUIRE(diagnostic.at("title") == "aaltitoad cannot parse the guard");
        REQUIRE(diagnostic.at("affectedElements") == nlohmann::json::array({"le1.guard"}));
    }
    THEN("open models count as they are edited, and the problems are cleared when they are gone") {
        client.notify("graph/didOpen", {{"buffer", fixed.buffer()}});
        auto before = client.messages.size();
        client.work();
        auto published = client.notifications("graph/publishDiagnostics", before);
        REQUIRE(published.size() == 1);
        REQUIRE(published.at(0) == nlohmann::json{{"key", "light.json"}, {"diagnostics", nlohmann::json::array()}});
        THEN("unchanged problems are not published again") {
            client.notify("graph/didChange", {{"key", "light.json"}, {"version", 2}, {"buffer", fixed.buffer()}});
            before = client.messages.size();
            client.work();
            REQUIRE(client.notifications("graph/publishDiagnostics", before).empty());
        }
        THEN("closing the model returns to its file") {
            client.notify("graph/didClose", {{"key", "light.json"}});
            client.work();
            REQUIRE(client.diagnostics().at("light.json") == std::vector<std::string>{aaltitoad::graphedit::problems::invalid_expression});
        }
    }
    THEN("saved files count once they changed") {
        project.write("light.json", fixed.file());
        client.notify("graph/didSave", {{"key", "light.json"}});
        client.work();
        REQUIRE(client.diagnostics().at("light.json").empty());
    }
}

SCENARIO("debugging with the language server", "[lsp]") {
    client_t client{};
    client.initialize();
    GIVEN("a runtime that stops on entry") {
        auto before = client.messages.size();
        auto result = client.result("debug/launch", launch_params(light_buffers(), true, nlohmann::json::object(),
                                                                  {{{"id", "b1"}, {"element", {{"model", "main.json"}, {"id", "m1"}}}},
                                                                   {{"id", "b2"}, {"element", {{"model", "main.json"}, {"id", "nothing"}}}}}));
        THEN("launching answers with the breakpoints, before the runtime stops") {
            REQUIRE(result.at("breakpoints").size() == 2);
            REQUIRE(result.at("breakpoints").at(0) == nlohmann::json{{"id", "b1"}, {"verified", true}});
            REQUIRE(result.at("breakpoints").at(1).at("verified") == false);
            REQUIRE(client.messages.at(before).contains("result"));
            auto stopped = client.notifications("debug/stopped", before);
            REQUIRE(stopped.size() == 1);
            REQUIRE(stopped.at(0).at("reason") == "entry");
            REQUIRE(stopped.at(0).at("state").at("choices").size() == 1);
        }
        THEN("steps stop at breakpoints") {
            before = client.messages.size();
            REQUIRE(client.result("debug/step", {{"choice", "c0"}}).is_null());
            auto stopped = client.notifications("debug/stopped", before);
            REQUIRE(stopped.size() == 1);
            REQUIRE(stopped.at(0).at("reason") == "breakpoint");
            REQUIRE(stopped.at(0).at("breakpoints") == nlohmann::json::array({"b1"}));
            REQUIRE(stopped.at(0).at("state").at("variables").at("count").at("value") == 1);
            REQUIRE(client.error_code("debug/step", {{"choice", "c9"}}) == aaltitoad::debugger::invalid_params);
        }
        THEN("continuing runs the runtime until it stops") {
            client.result("debug/setBreakpoints", {{"breakpoints", nlohmann::json::array()}});
            before = client.messages.size();
            REQUIRE(client.result("debug/continue").is_null());
            REQUIRE(client.notifications("debug/continued", before).size() == 1);
            REQUIRE(client.server.is_running());
            REQUIRE(client.error_code("debug/step") == aaltitoad::debugger::not_now);
            REQUIRE(client.error_code("debug/setVariables", {{"values", {{"count", 1}}}}) == aaltitoad::debugger::not_now);
            for(int i = 0; i < 100 && client.server.is_running(); i++)
                client.work();
            auto stopped = client.notifications("debug/stopped", before);
            REQUIRE(stopped.size() == 1);
            REQUIRE(stopped.at(0).at("reason") == "stuck");
            REQUIRE(stopped.at(0).at("state").at("variables").at("count").at("value") == 3);
        }
        THEN("the state can be changed and restored while the runtime is stopped") {
            auto first = client.result("debug/state").at("id");
            auto changed = client.result("debug/setVariables", {{"values", {{"count", 2}}}});
            REQUIRE(changed.at("variables").at("count").at("value") == 2);
            REQUIRE(changed.at("id") != first);
            auto moved = client.result("debug/setActive", {{"instance", "Main"}, {"active", {"m2"}}});
            REQUIRE(moved.at("instances").at(0).at("active") == nlohmann::json::array({"m2"}));
            auto restored = client.result("debug/restore", {{"state", first}});
            REQUIRE(restored.at("id") == first);
            REQUIRE(restored.at("variables").at("count").at("value") == 0);
            REQUIRE(client.error_code("debug/restore", {{"state", "s42"}}) == aaltitoad::debugger::invalid_params);
            REQUIRE(client.error_code("debug/setVariables", {{"values", {{"count", "two"}}}}) == aaltitoad::debugger::invalid_params);
            REQUIRE(client.error_code("debug/setActive", {{"instance", "Main"}}) == aaltitoad::debugger::invalid_params);
        }
        THEN("one session runs at a time, until it is disconnected") {
            REQUIRE(client.error_code("debug/launch", launch_params(light_buffers(), true)) == aaltitoad::debugger::not_now);
            before = client.messages.size();
            REQUIRE(client.result("debug/disconnect", {{"terminate", false}}).is_null());
            REQUIRE(client.messages.size() == before + 1);
            REQUIRE(client.error_code("debug/state") == aaltitoad::debugger::not_now);
            REQUIRE(client.error_code("debug/disconnect") == aaltitoad::debugger::not_now);
            client.result("debug/launch", launch_params(light_buffers(), true));
        }
    }
    GIVEN("a runtime that runs from the start") {
        auto before = client.messages.size();
        client.result("debug/launch", launch_params(endless_buffers(), false));
        THEN("it reports that it runs, and its state while it runs") {
            REQUIRE(client.notifications("debug/continued", before).size() == 1);
            REQUIRE(client.notifications("debug/stopped", before).empty());
            auto start = std::chrono::steady_clock::now();
            while(std::chrono::steady_clock::now() - start < std::chrono::milliseconds{350})
                client.work();
            auto live = client.notifications("debug/stateChanged", before);
            REQUIRE(live.size() >= 2);
            REQUIRE(live.size() <= 4);
            REQUIRE(client.result("debug/state").at("variables").at("count").at("value").get<int>() > 0);
            THEN("pausing stops it right away") {
                auto paused = client.messages.size();
                REQUIRE(client.result("debug/pause").is_null());
                auto stopped = client.notifications("debug/stopped", paused);
                REQUIRE(stopped.size() == 1);
                REQUIRE(stopped.at(0).at("reason") == "pause");
                REQUIRE_FALSE(client.server.is_running());
                REQUIRE(client.result("debug/pause").is_null());
                REQUIRE(client.notifications("debug/stopped", paused).size() == 1);
            }
        }
    }
    GIVEN("a runtime that takes its time") {
        client.result("debug/launch", launch_params(endless_buffers(), false, {{"delay", 200}}));
        THEN("it waits between its steps") {
            client.server.work();
            REQUIRE(client.result("debug/state").at("instances").at(0).at("active") == nlohmann::json::array({"m1"}));
            REQUIRE(client.server.next_work() > std::chrono::steady_clock::now() + std::chrono::milliseconds{100});
            client.result("debug/pause");
        }
    }
    GIVEN("networks that cannot run") {
        auto models = light_buffers();
        models.at(0).at("metadata")["is_main"] = "false";
        THEN("launching is refused, and says why") {
            auto response = client.request("debug/launch", launch_params(models, true));
            REQUIRE(response.at("error").at("code") == aaltitoad::debugger::invalid_params);
            REQUIRE(response.at("error").at("message").get<std::string>().find("aaltitoad cannot run the network: main.json") == 0);
            REQUIRE(client.error_code("debug/launch", launch_params(42, true)) == aaltitoad::debugger::invalid_params);
            REQUIRE(client.error_code("debug/launch", launch_params(light_buffers(), true, {{"policy", "fast"}})) == aaltitoad::debugger::invalid_params);
            REQUIRE(client.error_code("debug/attach", launch_params(light_buffers(), true)) == method_not_found);
        }
    }
}

#ifdef AALTITOAD_LS_PROGRAM
SCENARIO("the aaltitoad-ls program speaks the protocol on its standard input and output", "[lsp]") {
    aaltitoad::test::temporary_directory_t directory{};
    auto id = 0;
    auto message = [&id](const std::string& method, const nlohmann::json& params, bool request) {
        nlohmann::json json{{"jsonrpc", "2.0"}, {"method", method}, {"params", params}};
        if(request)
            json["id"] = ++id;
        return frame(json.dump());
    };
    // NOTE: one after the other, since the ids count the requests
    std::string input{};
    input += message("initialize", {{"processId", 1}, {"capabilities", nlohmann::json::object()}}, true);
    input += message("initialized", nlohmann::json::object(), false);
    input += message("debug/launch", launch_params(light_buffers(), true), true);
    input += message("debug/step", nlohmann::json::object(), true);
    input += message("debug/disconnect", {{"terminate", false}}, true);
    input += message("shutdown", nullptr, true);
    input += message("exit", nullptr, false);
    directory.write_text("input", input);
    auto command = std::filesystem::path{AALTITOAD_LS_PROGRAM}.make_preferred().string() + " --verbosity 0 < " + (directory.path / "input").string() + " > " +
                   (directory.path / "output").string();
    auto status = std::system(command.c_str());
    THEN("it answers every request, in order, and exits cleanly") {
#ifdef _WIN32
        REQUIRE(status == 0);
#else
        REQUIRE(WIFEXITED(status));
        REQUIRE(WEXITSTATUS(status) == 0);
#endif
        std::ifstream file{directory.path / "output"};
        std::string output{std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>{}};
        message_reader reader{};
        reader.feed(output);
        std::vector<nlohmann::json> responses{};
        std::vector<std::string> notifications{};
        while(auto next = reader.next()) {
            auto json = nlohmann::json::parse(next.value());
            if(json.contains("id"))
                responses.push_back(json);
            else
                notifications.push_back(json.at("method").get<std::string>());
        }
        REQUIRE(responses.size() == 5);
        for(std::size_t i = 0; i < responses.size(); i++) {
            REQUIRE(responses[i].at("id") == static_cast<int>(i + 1));
            REQUIRE(responses[i].contains("result"));
        }
        REQUIRE(responses[0].at("result").at("language") == "NTTA");
        REQUIRE(notifications == std::vector<std::string>{"debug/stopped", "debug/stopped"});
    }
}

SCENARIO("the aaltitoad-ls program does its work while it waits for messages", "[lsp]") {
    aaltitoad::test::temporary_directory_t directory{};
    auto id = 0;
    auto message = [&id](const std::string& method, const nlohmann::json& params, bool request) {
        nlohmann::json json{{"jsonrpc", "2.0"}, {"method", method}, {"params", params}};
        if(request)
            json["id"] = ++id;
        return frame(json.dump());
    };
    auto command = std::filesystem::path{AALTITOAD_LS_PROGRAM}.make_preferred().string() + " --verbosity 0 > " + (directory.path / "output").string();
#ifdef _WIN32
    auto* server = _popen(command.c_str(), "wb");
#else
    auto* server = popen(command.c_str(), "w");
#endif
    REQUIRE(server != nullptr);
    auto send = [server](const std::string& data) {
        std::fwrite(data.data(), 1, data.size(), server);
        std::fflush(server);
    };
    auto output = [&directory]() {
        std::ifstream file{directory.path / "output"};
        return std::string{std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>{}};
    };
    WHEN("a runtime runs with a delay between its steps, and no message arrives") {
        send(message("initialize", {{"processId", 1}, {"capabilities", nlohmann::json::object()}}, true));
        send(message("initialized", nlohmann::json::object(), false));
        send(message("debug/launch", launch_params(light_buffers(), false, {{"delay", 100}}), true));
        // NOTE: the light network is stuck after three steps (300ms)
        auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds{10};
        while(output().find("debug/stopped") == std::string::npos && std::chrono::steady_clock::now() < deadline)
            std::this_thread::sleep_for(std::chrono::milliseconds{50});
        auto stopped = output().find("debug/stopped") != std::string::npos;
        send(message("shutdown", nullptr, true));
        send(message("exit", nullptr, false));
#ifdef _WIN32
        auto status = _pclose(server);
#else
        auto status = pclose(server);
#endif
        THEN("the runtime takes its steps until it stops, and the program exits cleanly") {
            REQUIRE(stopped);
#ifdef _WIN32
            REQUIRE(status == 0);
#else
            REQUIRE(WIFEXITED(status));
            REQUIRE(WEXITSTATUS(status) == 0);
#endif
        }
    }
}
#endif
