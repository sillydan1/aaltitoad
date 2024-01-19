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
#include "cli/lsp/model.h"
#include "lsp.pb.h"
#include "lsp_server.h"
#include <exception>
#include <future>
#include <grpc/support/log.h>
#include <grpcpp/support/status.h>
#include <spdlog/spdlog.h>
#include <unistd.h>
#include <variant>

namespace aaltitoad::lsp::proto {
    LanguageServerImpl::LanguageServerImpl(int port, const std::string& semver) : running{false}, port{port}, semver{semver}, diagnostics_callback{}, notifications_callback{}, progress_callback{} {

    }

    LanguageServerImpl::~LanguageServerImpl() {
        diagnostics_callback.reset();
        notifications_callback.reset();
        progress_callback.reset();
    }

    void LanguageServerImpl::start() {
        auto ss = std::stringstream{} << "0.0.0.0:" << port;
        grpc::ServerBuilder builder;
        builder.AddListeningPort(ss.str(), grpc::InsecureServerCredentials());
        builder.RegisterService(this);
        std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
        spdlog::info("language server listening on {}", ss.str());
        server->Wait();
    }

    auto LanguageServerImpl::HandleDiff(grpc::ServerContext* server_context, const Diff* diff, Empty* result) -> grpc::Status {
        try {
            // NOTE: BOOKMARK: You're converting to model objects, so you can feed them into the builder, so you can get compiler errors
            progress_start("diff for '" + diff->buffername() + "' received");
            if(!network.components.contains(diff->buffername())) {
                network.components[diff->buffername()] = tta_t{};
                progress(diff->buffername() + " opened");
            }
            for(auto x : diff->vertexadditions()) {
                auto json = nlohmann::json::parse(x.jsonencoding());
                lsp::vertex_t vert{};
                lsp::from_json(json, vert);
                notify_info(x.id());
                std::visit(ya::overload(
                    [this](const lsp::nail_t& n){ notify_info("nail expression: " + n.expression); },
                    [this](const lsp::location_t& l){ notify_info("location nickname: " + l.nickname); }
                ), static_cast<const lsp::vertex_t&>(vert));
            }
            for(auto x : diff->edgeadditions()) {
                auto json = nlohmann::json::parse(x.jsonencoding());
            }
            for(auto x : diff->vertexdeletions()) {
                auto json = nlohmann::json::parse(x.jsonencoding());
            }
            for(auto x : diff->edgedeletions()) {
                auto json = nlohmann::json::parse(x.jsonencoding());
            }
            progress_end("done");
            return grpc::Status::OK;
        } catch(const std::exception& e) {
            spdlog::error("something went wrong: {}", e.what());
            return grpc::Status(grpc::StatusCode::INTERNAL, e.what());
        } catch(...) {
            spdlog::error("unknown error");
            return grpc::Status(grpc::StatusCode::INTERNAL, "unknown error");
        }
    }

    auto LanguageServerImpl::GetServerInfo(grpc::ServerContext* server_context, const Empty* empty, ServerInfo* result) -> grpc::Status {
        result->set_name("aaltitoad-ls");
        result->set_language("hawk");
        result->set_semanticversion(semver);
        return grpc::Status::OK;
    }

    auto LanguageServerImpl::GetDiagnostics(grpc::ServerContext* server_context, const Empty* empty, grpc::ServerWriter<DiagnosticsList>* writer) -> grpc::Status {
        diagnostics_callback = [&writer](const DiagnosticsList& d){ writer->Write(d); };
        // NOTE: we now sleep forever, because we dont want to actually exit this call ever
        std::promise<void>().get_future().wait();
        return grpc::Status::OK;
    }

    auto LanguageServerImpl::GetNotifications(grpc::ServerContext* server_context, const Empty* empty, grpc::ServerWriter<Notification>* writer) -> grpc::Status {
        notifications_callback = [&writer](const Notification& n){ writer->Write(n); };
        // NOTE: we now sleep forever, because we dont want to actually exit this call ever
        std::promise<void>().get_future().wait();
        return grpc::Status::OK;
    }

    auto LanguageServerImpl::GetProgress(grpc::ServerContext* server_context, const Empty* empty, grpc::ServerWriter<ProgressReport>* writer) -> grpc::Status {
        progress_callback = [&writer](const ProgressReport& p){ writer->Write(p); };
        // NOTE: we now sleep forever, because we dont want to actually exit this call ever
        std::promise<void>().get_future().wait();
        return grpc::Status::OK;
    }

    void LanguageServerImpl::progress_start(const std::string& message) {
        progress(ProgressReportType::PROGRESS_BEGIN, message);
    }

    void LanguageServerImpl::progress(const std::string& message) {
        progress(ProgressReportType::PROGRESS_STATUS, message);
    }

    void LanguageServerImpl::progress_end(const std::string& message) {
        progress(ProgressReportType::PROGRESS_END, message);
    }

    void LanguageServerImpl::progress_end_fail(const std::string& message) {
        progress(ProgressReportType::PROGRESS_END_FAIL, message);
    }

    void LanguageServerImpl::progress(const ProgressReportType& type, const std::string& message) {
        if(!progress_callback.has_value())
            return;
        ProgressReport result{};
        result.set_type(type);
        result.set_message(message);
        result.set_title("aaltitoad-ls");
        result.set_token("ls-info");
        progress_callback.value()(result);
    }

    void LanguageServerImpl::notify_error(const std::string& message) {
        notify(NotificationLevel::NOTIFICATION_ERROR, message);
    }

    void LanguageServerImpl::notify_info(const std::string& message) {
        notify(NotificationLevel::NOTIFICATION_INFO, message);
    }

    void LanguageServerImpl::notify_warning(const std::string& message) {
        notify(NotificationLevel::NOTIFICATION_WARNING, message);
    }

    void LanguageServerImpl::notify_debug(const std::string& message) {
        notify(NotificationLevel::NOTIFICATION_DEBUG, message);
    }

    void LanguageServerImpl::notify_trace(const std::string& message) {
        notify(NotificationLevel::NOTIFICATION_TRACE, message);
    }

    void LanguageServerImpl::notify(const NotificationLevel& level, const std::string& message) {
        if(!notifications_callback.has_value())
            return;
        Notification result{};
        result.set_level(level);
        result.set_message(message);
        notifications_callback.value()(result);
    }
}
