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
#include "language_server.h"
#include "json_rpc.h"
#include "query.h"
#include <parser/graphedit/problems.h>
#include <spdlog/spdlog.h>
#include <limits>
#include <set>

namespace aaltitoad::lsp {
    namespace {
        using debugger::request_error;
        using clock = std::chrono::steady_clock;

        auto to_string(graphedit::severity_t severity) -> std::string {
            switch(severity) {
                case graphedit::severity_t::hint: return "hint";
                case graphedit::severity_t::info: return "info";
                case graphedit::severity_t::warning: return "warning";
                case graphedit::severity_t::error: return "error";
            }
            return "error";
        }

        auto to_json(const graphedit::problem_t& problem) -> nlohmann::json {
            return {{"lintIdentifier", problem.identifier}, {"severity", to_string(problem.severity)},
                    {"title", problem.title}, {"message", problem.message}, {"affectedElements", problem.elements}};
        }

        auto string_param(const nlohmann::json& params, const char* key) -> std::string {
            auto it = params.find(key);
            if(it == params.end() || !it->is_string())
                throw request_error(invalid_params, std::string{"'"} + key + "' must be a string");
            return it->get<std::string>();
        }

        auto breakpoints_of(const nlohmann::json& params) -> std::vector<debugger::breakpoint_t> {
            std::vector<debugger::breakpoint_t> result{};
            auto it = params.find("breakpoints");
            if(it == params.end() || it->is_null())
                return result;
            if(!it->is_array())
                throw request_error(invalid_params, "'breakpoints' must be an array");
            for(auto& breakpoint : *it)
                result.push_back(debugger::breakpoint_from_json(breakpoint));
            return result;
        }

        auto statuses_json(const std::vector<debugger::breakpoint_status_t>& statuses) -> nlohmann::json {
            auto result = nlohmann::json::array();
            for(auto& status : statuses)
                result.push_back(debugger::to_json(status));
            return {{"breakpoints", result}};
        }

        const std::set<std::string> debug_methods{"debug/launch", "debug/attach", "debug/setBreakpoints", "debug/continue",
                                                  "debug/pause", "debug/step", "debug/state", "debug/setVariables",
                                                  "debug/setActive", "debug/restore", "debug/disconnect"};
    }

    language_server::language_server(writer_t writer, std::string version)
            : writer_{std::move(writer)}, version_{std::move(version)} {}

    void language_server::handle(const std::string& text) {
        nlohmann::json message{};
        try {
            message = nlohmann::json::parse(text);
        } catch(const std::exception& e) {
            send({{"jsonrpc", "2.0"}, {"id", nullptr}, {"error", {{"code", parse_error}, {"message", e.what()}}}});
            return;
        }
        auto method = message.is_object() ? message.find("method") : message.end();
        if(!message.is_object() || method == message.end() || !method->is_string()) {
            // NOTE: responses are ignored, as the server sends no requests
            if(message.is_object() && message.contains("id") && (message.contains("result") || message.contains("error")))
                return;
            auto id = message.is_object() && message.contains("id") ? message.at("id") : nlohmann::json{};
            send({{"jsonrpc", "2.0"}, {"id", id}, {"error", {{"code", invalid_request}, {"message", "not a JSON-RPC request"}}}});
            return;
        }
        auto params = message.contains("params") ? message.at("params") : nlohmann::json::object();
        if(!message.contains("id")) {
            try {
                notification(method->get<std::string>(), params);
            } catch(const std::exception& e) {
                spdlog::warn("invalid {} notification: {}", method->get<std::string>(), e.what());
            }
            return;
        }
        nlohmann::json response{{"jsonrpc", "2.0"}, {"id", message.at("id")}};
        handling_request_ = true;
        auto answered = true;
        try {
            auto result = request(message.at("id"), method->get<std::string>(), params);
            if(result)
                response["result"] = std::move(result.value());
            else
                answered = false;
        } catch(const request_error& e) {
            response["error"] = {{"code", e.code}, {"message", e.what()}};
        } catch(const nlohmann::json::exception& e) {
            response["error"] = {{"code", invalid_params}, {"message", e.what()}};
        } catch(const std::exception& e) {
            spdlog::error("{} failed: {}", method->get<std::string>(), e.what());
            response["error"] = {{"code", internal_error}, {"message", e.what()}};
        }
        handling_request_ = false;
        if(answered)
            send(response);
        auto pending = std::move(pending_);
        pending_.clear();
        for(auto& notification : pending)
            send(notification);
    }

    auto language_server::should_exit() const -> bool {
        return exit_;
    }

    auto language_server::exit_code() const -> int {
        return shutdown_ ? 0 : 1;
    }

    auto language_server::is_running() const -> bool {
        return runtime_ && running_;
    }

    auto language_server::is_verifying() const -> bool {
        return search_ != nullptr;
    }

    auto language_server::request(const nlohmann::json& id, const std::string& method, const nlohmann::json& params) -> std::optional<nlohmann::json> {
        if(shutdown_)
            throw request_error(invalid_request, "the server is shutting down");
        if(method == "initialize")
            return initialize(params);
        if(!initialized_)
            throw request_error(server_not_initialized, "the server has not been initialized");
        if(method == "shutdown") {
            shutdown_ = true;
            end_session();
            if(search_)
                answer({{"error", {{"code", request_cancelled}, {"message", "the server shuts down"}}}});
            return nullptr;
        }
        if(method.starts_with("debug/"))
            return debug_request(method, params);
        if(method == "verify/run") {
            verify(id, params);
            return std::nullopt;
        }
        throw request_error(method_not_found, "aaltitoad-ls does not know the method '" + method + "'");
    }

    void language_server::notification(const std::string& method, const nlohmann::json& params) {
        if(method == "exit") {
            exit_ = true;
            return;
        }
        if(!initialized_)
            return;
        if(method == "graph/didOpen") {
            auto model = graphedit::read_buffer(params.at("buffer"));
            buffers_[model.key] = std::move(model);
            schedule_analysis();
        } else if(method == "graph/didChange") {
            if(!params.contains("buffer")) {
                spdlog::warn("graph/didChange without the model (aaltitoad-ls asks for complete models)");
                return;
            }
            auto model = graphedit::read_buffer(params.at("buffer"));
            buffers_[model.key] = std::move(model);
            schedule_analysis();
        } else if(method == "graph/didSave") {
            schedule_analysis();
        } else if(method == "graph/didClose") {
            buffers_.erase(params.at("key").get<std::string>());
            schedule_analysis();
        } else if(method == "verify/cancel") {
            cancel_verification(params.at("id").get<std::string>());
        }
    }

    auto language_server::initialize(const nlohmann::json& params) -> nlohmann::json {
        if(initialized_)
            throw request_error(invalid_request, "the server has been initialized already");
        initialized_ = true;
        auto project = params.find("project");
        if(project != params.end() && project->is_object() && project->contains("path") && project->at("path").is_string() &&
           !project->at("path").get<std::string>().empty()) {
            std::vector<std::string> excluded{};
            if(auto exclude = project->find("excludeFiles"); exclude != project->end() && exclude->is_array())
                for(auto& pattern : *exclude)
                    if(pattern.is_string())
                        excluded.push_back(pattern.get<std::string>());
            project_.emplace(project->at("path").get<std::string>(), excluded);
        }
        analysis_due_ = clock::now();
        return {{"serverInfo", {{"name", "aaltitoad-ls"}, {"version", version_}}},
                {"language", graphedit::syntax_name},
                {"capabilities", {{"sync", "full"}, {"diagnostics", true}, {"progress", false}, {"notifications", false},
                                  {"debug", {{"launch", true}, {"attach", false}, {"chooseSteps", true}, {"history", true},
                                             {"setVariables", true}, {"setActive", true}, {"breakpoints", true},
                                             {"conditionBreakpoints", true}, {"hitCounts", true}, {"liveState", true}}},
                                  {"verification", {{"traces", true}, {"cancel", true}, {"examples", {
                                          {{"query", "E F Main@done"}, {"description", "Can the main template reach its location 'done'?"}},
                                          {{"query", "A G count <= 10"}, {"description", "Does the variable count stay at most 10?"}},
                                          {{"query", "E F Main.lamp(1)@on && !button.1"},
                                           {"description", "Can the instance lamp(1) be in its location 'on' while button.1 is false?"}}}}}}}}};
    }

    auto language_server::debug_request(const std::string& method, const nlohmann::json& params) -> nlohmann::json {
        if(!debug_methods.contains(method))
            throw request_error(method_not_found, "aaltitoad-ls does not know the method '" + method + "'");
        if(method == "debug/attach")
            throw request_error(method_not_found, "aaltitoad-ls cannot attach to runtimes (yet)");
        if(method == "debug/launch")
            return launch(params);
        if(!runtime_)
            throw request_error(debugger::not_now, "there is no debug session");
        if(method == "debug/disconnect") {
            end_session();
            return nullptr;
        }
        if(method == "debug/setBreakpoints")
            return statuses_json(runtime_->set_breakpoints(breakpoints_of(params)));
        if(method == "debug/state")
            return runtime_->state();
        if(method == "debug/continue") {
            if(!running_) {
                running_ = true;
                next_step_ = clock::now();
                last_live_state_ = next_step_;
                notify("debug/continued", nlohmann::json::object());
            }
            return nullptr;
        }
        if(method == "debug/pause") {
            if(running_) {
                running_ = false;
                report_stop({debugger::stop_reason::pause, {}, {}});
            }
            return nullptr;
        }
        if(running_)
            throw request_error(debugger::not_now, "the runtime runs: pause it first");
        if(method == "debug/step") {
            std::optional<std::string> choice{};
            if(auto it = params.find("choice"); it != params.end() && !it->is_null()) {
                if(!it->is_string())
                    throw request_error(invalid_params, "'choice' must be a string");
                choice = it->get<std::string>();
            }
            report_stop(runtime_->step(choice));
            return nullptr;
        }
        if(method == "debug/setVariables") {
            if(!params.contains("values"))
                throw request_error(invalid_params, "'values' is missing");
            runtime_->set_variables(params.at("values"));
            return runtime_->state();
        }
        if(method == "debug/setActive") {
            if(!params.contains("active"))
                throw request_error(invalid_params, "'active' is missing");
            runtime_->set_active(string_param(params, "instance"), params.at("active"));
            return runtime_->state();
        }
        // debug/restore
        runtime_->restore(string_param(params, "state"));
        return runtime_->state();
    }

    auto language_server::launch(const nlohmann::json& params) -> nlohmann::json {
        if(runtime_)
            throw request_error(debugger::not_now, "a debug session runs already");
        nlohmann::json arguments{};
        if(auto configuration = params.find("configuration"); configuration != params.end() && configuration->is_object())
            if(auto it = configuration->find("arguments"); it != configuration->end())
                arguments = *it;
        auto parsed_arguments = debugger::arguments_from_json(arguments);
        auto breakpoints = breakpoints_of(params);
        auto network = network_of(params);
        runtime_ = std::make_unique<debugger::runtime>(std::move(network), parsed_arguments);
        auto statuses = runtime_->set_breakpoints(breakpoints);
        auto stop_on_entry = params.contains("stopOnEntry") && params.at("stopOnEntry").is_boolean()
                ? params.at("stopOnEntry").get<bool>() : true;
        if(stop_on_entry) {
            report_stop({debugger::stop_reason::entry, {}, {}});
        } else {
            running_ = true;
            next_step_ = clock::now();
            last_live_state_ = next_step_;
            notify("debug/continued", nlohmann::json::object());
        }
        return statuses_json(statuses);
    }

    auto language_server::network_of(const nlohmann::json& params) -> graphedit::network_t {
        auto models_json = params.find("models");
        if(models_json == params.end() || !models_json->is_array())
            throw request_error(invalid_params, "'models' must be an array of models");
        std::map<std::string, graphedit::ntta_model_t> models{};
        for(auto& buffer : *models_json) {
            try {
                auto model = graphedit::read_buffer(buffer);
                models[model.key] = std::move(model);
            } catch(const std::invalid_argument& e) {
                throw request_error(invalid_params, e.what());
            }
        }
        auto network = graphedit::build_network(std::move(models));
        if(network.automata)
            return network;
        std::vector<std::string> reasons{};
        for(auto& [key, problems] : network.problems)
            for(auto& problem : problems)
                if(problem.severity == graphedit::severity_t::error && reasons.size() < 3)
                    reasons.push_back(key + ": " + problem.title + " (" + problem.message + ")");
        std::string text{"aaltitoad cannot run the network"};
        for(std::size_t i = 0; i < reasons.size(); i++)
            text += (i == 0 ? ": " : "; ") + reasons[i];
        throw request_error(invalid_params, text);
    }

    void language_server::verify(const nlohmann::json& id, const nlohmann::json& params) {
        if(search_)
            throw request_error(request_failed, "another query is being verified (aaltitoad-ls answers one at a time)");
        auto run = string_param(params, "id");
        auto text = string_param(params, "query");
        auto max_states = default_max_states;
        if(auto options = params.find("options"); options != params.end() && options->is_object())
            if(auto it = options->find("maxStates"); it != options->end() && !it->is_null()) {
                if(!it->is_number_integer() || it->get<long long>() < 1)
                    throw request_error(invalid_params, "the option maxStates must be a whole number that is at least 1");
                max_states = it->get<std::size_t>();
            }
        auto network = network_of(params);
        std::optional<forward_reachability_searcher::query_solution_t> query{};
        try {
            query = compile_query(text, network);
        } catch(const std::invalid_argument& e) {
            throw request_error(invalid_params, e.what());
        }
        spdlog::info("verifying '{}' ({})", text, run);
        search_ = std::make_unique<search>(std::move(network), std::move(query.value()), max_states);
        search_request_ = id;
        search_run_ = run;
        last_progress_ = clock::now();
    }

    void language_server::cancel_verification(const std::string& run) {
        if(!search_ || run != search_run_)
            return;
        spdlog::info("verifying {} was cancelled", run);
        answer({{"error", {{"code", request_cancelled}, {"message", "the verification was cancelled"}}}});
    }

    void language_server::search_further() {
        if(!search_)
            return;
        if(search_->run(clock::now() + run_slice)) {
            if(search_->status() == search::status_t::failed)
                answer({{"error", {{"code", request_failed}, {"message", search_->message()}}}});
            else
                answer({{"result", search_->result()}});
            return;
        }
        if(clock::now() - last_progress_ < progress_interval)
            return;
        last_progress_ = clock::now();
        notify("verify/progress", {{"id", search_run_}, {"message", std::to_string(search_->states()) + " states searched, " +
                                                              std::to_string(search_->waiting()) + " waiting"}});
    }

    void language_server::answer(nlohmann::json response) {
        response["jsonrpc"] = "2.0";
        response["id"] = search_request_;
        search_.reset();
        search_request_ = nullptr;
        search_run_.clear();
        // NOTE: the answer of a request that is being handled (e.g. a run cancelled by shutdown) waits for its response
        notify_message(std::move(response));
    }

    void language_server::report_stop(const debugger::stop_t& stop) {
        nlohmann::json params{{"reason", debugger::to_string(stop.reason)}};
        if(!stop.breakpoints.empty())
            params["breakpoints"] = stop.breakpoints;
        if(!stop.description.empty())
            params["description"] = stop.description;
        params["state"] = runtime_->state();
        notify("debug/stopped", std::move(params));
    }

    void language_server::end_session() {
        runtime_.reset();
        running_ = false;
    }

    auto language_server::next_work() const -> std::optional<clock::time_point> {
        std::optional<clock::time_point> result = analysis_due_;
        if(runtime_ && running_ && (!result || next_step_ < result.value()))
            result = next_step_;
        if(search_)
            result = clock::now();
        return result;
    }

    void language_server::work() {
        auto now = clock::now();
        if(analysis_due_ && now >= analysis_due_.value())
            analyze();
        search_further();
        if(!runtime_ || !running_ || now < next_step_)
            return;
        std::optional<debugger::stop_t> stop{};
        auto delay = runtime_->arguments().delay;
        try {
            if(delay > 0) {
                stop = runtime_->run(1, now);
                next_step_ = now + std::chrono::milliseconds{delay};
            } else {
                stop = runtime_->run(std::numeric_limits<std::size_t>::max(), now + run_slice);
                next_step_ = clock::now();
            }
        } catch(const std::exception& e) {
            stop = debugger::stop_t{debugger::stop_reason::error, {}, e.what()};
        }
        if(stop) {
            running_ = false;
            report_stop(stop.value());
        } else if(clock::now() - last_live_state_ >= live_state_interval) {
            last_live_state_ = clock::now();
            notify("debug/stateChanged", {{"state", runtime_->state(true)}});
        }
    }

    void language_server::schedule_analysis() {
        analysis_due_ = clock::now() + analysis_delay;
    }

    void language_server::analyze() {
        analysis_due_.reset();
        auto started = clock::now();
        std::map<std::string, graphedit::ntta_model_t> models{};
        std::map<std::string, std::string> unreadable{};
        if(project_) {
            project_->scan();
            models = project_->models();
            unreadable = project_->unreadable();
        }
        for(auto& [key, model] : buffers_) {
            models[key] = model;
            unreadable.erase(key);
        }
        auto count = models.size();
        auto network = graphedit::build_network(std::move(models));
        spdlog::info("analyzed {} models in {} ms: {}", count,
                     std::chrono::duration_cast<std::chrono::milliseconds>(clock::now() - started).count(),
                     network.automata ? "aaltitoad can run the network" : "aaltitoad cannot run the network");
        std::map<std::string, nlohmann::json> diagnostics{};
        for(auto& [key, problems] : network.problems)
            for(auto& problem : problems)
                diagnostics[key].push_back(to_json(problem));
        for(auto& [key, message] : unreadable)
            diagnostics[key].push_back(to_json({graphedit::problems::unreadable_model, graphedit::severity_t::warning,
                                                "aaltitoad cannot read the model", message, {}}));
        for(auto& [key, list] : diagnostics) {
            auto published = published_.find(key);
            if(published != published_.end() && published->second == list)
                continue;
            send({{"jsonrpc", "2.0"}, {"method", "graph/publishDiagnostics"}, {"params", {{"key", key}, {"diagnostics", list}}}});
        }
        for(auto& [key, list] : published_)
            if(!diagnostics.contains(key))
                send({{"jsonrpc", "2.0"}, {"method", "graph/publishDiagnostics"},
                      {"params", {{"key", key}, {"diagnostics", nlohmann::json::array()}}}});
        published_ = std::move(diagnostics);
    }

    void language_server::notify(const std::string& method, nlohmann::json params) {
        notify_message({{"jsonrpc", "2.0"}, {"method", method}, {"params", std::move(params)}});
    }

    void language_server::notify_message(nlohmann::json message) {
        if(handling_request_)
            pending_.push_back(std::move(message));
        else
            send(message);
    }

    void language_server::send(const nlohmann::json& message) {
        writer_(message.dump(-1, ' ', false, nlohmann::json::error_handler_t::replace));
    }
}
