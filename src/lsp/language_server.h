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
#ifndef AALTITOAD_LSP_LANGUAGE_SERVER_H
#define AALTITOAD_LSP_LANGUAGE_SERVER_H
#include "project.h"
#include <debugger/runtime.h>
#include <chrono>
#include <functional>
#include <memory>

namespace aaltitoad::lsp {
    // The language server of aaltitoad for graphedit: speaks the graph language server protocol of graphedit
    // (docs/language-server-protocol.md of graphedit) for the NTTA models of graphedit-ntta. It publishes the problems
    // that aaltitoad finds in the network of a project (its model files, and the open models as they are edited), and
    // debugs networks (docs/debugging.md of graphedit): it launches runtimes for them, which take the steps of the
    // network, stop at breakpoints, and change and restore states.
    //
    // The server does not wait for anything: messages are handled as they arrive, and the work that is due (analyzing
    // the models after they changed, taking the steps of a runtime that runs) is done when the transport asks for it
    // (see next_work and work), so that requests such as debug/pause are read while the runtime runs.
    class language_server {
    public:
        // Sends a message (its json text, without framing)
        using writer_t = std::function<void(const std::string& message)>;
        // How long to wait after a change of the models before analyzing them, since changes come in bursts
        static constexpr std::chrono::milliseconds analysis_delay{100};
        // How often the state is reported while the runtime runs (debug/stateChanged)
        static constexpr std::chrono::milliseconds live_state_interval{100};
        // How long the runtime runs at a time, before the server reads messages again
        static constexpr std::chrono::milliseconds run_slice{20};

        language_server(writer_t writer, std::string version);

        // Handle a message: a request (which is answered), a notification or a response
        void handle(const std::string& message);
        // Whether the client said exit, and the exit code of the server (0 if it was shut down first)
        auto should_exit() const -> bool;
        auto exit_code() const -> int;
        // When work is due (std::nullopt: nothing to do until a message arrives)
        auto next_work() const -> std::optional<std::chrono::steady_clock::time_point>;
        // Do the work that is due
        void work();
        // Whether a debug session runs its runtime (for tests)
        auto is_running() const -> bool;

    private:
        auto request(const std::string& method, const nlohmann::json& params) -> nlohmann::json;
        void notification(const std::string& method, const nlohmann::json& params);
        auto initialize(const nlohmann::json& params) -> nlohmann::json;
        auto debug_request(const std::string& method, const nlohmann::json& params) -> nlohmann::json;
        auto launch(const nlohmann::json& params) -> nlohmann::json;
        void schedule_analysis();
        void analyze();
        void report_stop(const debugger::stop_t& stop);
        void end_session();
        // Send a notification: after the response of the request that is handled (if any), so that requests are
        // answered before the notifications that report their outcome
        void notify(const std::string& method, nlohmann::json params);
        void send(const nlohmann::json& message);

        writer_t writer_;
        std::string version_;
        bool initialized_{false};
        bool shutdown_{false};
        bool exit_{false};
        std::optional<project_files> project_{};
        // the open models, by key
        std::map<std::string, graphedit::ntta_model_t> buffers_{};
        // the diagnostics that were published last, by key (keys without diagnostics are left out)
        std::map<std::string, nlohmann::json> published_{};
        std::optional<std::chrono::steady_clock::time_point> analysis_due_{};
        // notifications that wait for the response of the request that is handled
        bool handling_request_{false};
        std::vector<nlohmann::json> pending_{};
        // the debug session
        std::unique_ptr<debugger::runtime> runtime_{};
        bool running_{false};
        std::chrono::steady_clock::time_point next_step_{};
        std::chrono::steady_clock::time_point last_live_state_{};
    };
}

#endif //AALTITOAD_LSP_LANGUAGE_SERVER_H
