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
#ifndef AALTITOAD_LSP_LANGUAGE_SERVER_H
#define AALTITOAD_LSP_LANGUAGE_SERVER_H
#include "project.h"
#include "search.h"
#include <debugger/runtime.h>
#include <chrono>
#include <functional>
#include <memory>

namespace aaltitoad::lsp {
    // The language server of aaltitoad for graphedit: speaks the graph language server protocol of graphedit
    // (docs/language-server-protocol.md of graphedit) for the NTTA models of graphedit-ntta. It publishes the problems
    // that aaltitoad finds in the network of a project (its model files, and the open models as they are edited),
    // debugs networks (docs/debugging.md of graphedit): it launches runtimes for them, which take the steps of the
    // network, stop at breakpoints, and change and restore states, and answers queries about networks
    // (docs/verification.md of graphedit, see query_t and search).
    //
    // The server does not wait for anything: messages are handled as they arrive, and the work that is due (analyzing
    // the models after they changed, taking the steps of a runtime that runs, searching the states for the answer to a
    // query) is done when the transport asks for it (see next_work and work), so that requests such as debug/pause and
    // verify/cancel are read meanwhile. verify/run is answered when its search ends.
    class language_server {
    public:
        // Sends a message (its json text, without framing)
        using writer_t = std::function<void(const std::string& message)>;
        // How long to wait after a change of the models before analyzing them, since changes come in bursts
        static constexpr std::chrono::milliseconds analysis_delay{100};
        // How often the state is reported while the runtime runs (debug/stateChanged)
        static constexpr std::chrono::milliseconds live_state_interval{100};
        // How long the runtime runs (or the search searches) at a time, before the server reads messages again
        static constexpr std::chrono::milliseconds run_slice{20};
        // How often the progress of a search is reported (verify/progress)
        static constexpr std::chrono::milliseconds progress_interval{100};
        // How many states a search finds at most, unless the option maxStates of the query says otherwise
        static constexpr std::size_t default_max_states = 100000;

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
        // Whether a debug session runs its runtime, and whether a query is being verified (for tests)
        auto is_running() const -> bool;
        auto is_verifying() const -> bool;

    private:
        // The result of a request, or std::nullopt if it is answered later (verify/run, see answer)
        auto request(const nlohmann::json& id, const std::string& method, const nlohmann::json& params) -> std::optional<nlohmann::json>;
        void notification(const std::string& method, const nlohmann::json& params);
        auto initialize(const nlohmann::json& params) -> nlohmann::json;
        auto debug_request(const std::string& method, const nlohmann::json& params) -> nlohmann::json;
        auto launch(const nlohmann::json& params) -> nlohmann::json;
        // The network of the models of the params of debug/launch or verify/run. Throws request_error (invalid_params)
        // if the models cannot be read or aaltitoad cannot run their network.
        auto network_of(const nlohmann::json& params) -> graphedit::network_t;
        // Start answering the query of verify/run (the request with the id)
        void verify(const nlohmann::json& id, const nlohmann::json& params);
        // Stop answering the query of a run (verify/cancel)
        void cancel_verification(const std::string& run);
        // Search for the answer to the query, and answer verify/run when it is known
        void search_further();
        // Answer verify/run, and forget its search
        void answer(nlohmann::json response);
        void schedule_analysis();
        void analyze();
        void report_stop(const debugger::stop_t& stop);
        void end_session();
        // Send a notification: after the response of the request that is handled (if any), so that requests are
        // answered before the notifications that report their outcome
        void notify(const std::string& method, nlohmann::json params);
        // Send a message after the response of the request that is handled (if any)
        void notify_message(nlohmann::json message);
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
        // the search for the answer to a query, the request of verify/run that waits for it and the id of the run
        std::unique_ptr<search> search_{};
        nlohmann::json search_request_{};
        std::string search_run_{};
        std::chrono::steady_clock::time_point last_progress_{};
    };
}

#endif //AALTITOAD_LSP_LANGUAGE_SERVER_H
