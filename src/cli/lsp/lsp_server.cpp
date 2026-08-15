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
#include "lsp.pb.h"
#include "lsp_server.h"
#include <future>
#include <grpc/support/log.h>
#include <grpcpp/support/status.h>
#include <spdlog/spdlog.h>
#include <sys/wait.h>
#include <unistd.h>

namespace aaltitoad::lsp::proto {
    LanguageServerImpl::LanguageServerImpl(int port, const std::string& semver, const std::shared_ptr<plugin::parser>& parser) : running{false}, port{port}, semver{semver}, diagnostics_callback{}, notifications_callback{}, progress_callback{}, parser{parser} {

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

    auto LanguageServerImpl::GetServerInfo(grpc::ServerContext* server_context, const Empty* empty, ServerInfo* result) -> grpc::Status {
        result->set_name("aaltitoad-lsp");
        result->set_language("hawk");
        result->set_semanticversion(semver);
        result->add_capabilities(Capability::CAPABILITY_PROJECT);
        result->add_capabilities(Capability::CAPABILITY_PROGRESS);
        result->add_capabilities(Capability::CAPABILITY_DIAGNOSTICS);
        result->add_capabilities(Capability::CAPABILITY_NOTIFICATIONS);
        return grpc::Status::OK;
    }

    auto LanguageServerImpl::ProjectOpened(grpc::ServerContext* server_context, const Project* project, Empty* result) -> grpc::Status {
        try {
            progress_start("opening project: " + project->name());
            std::vector<std::string> exclude_files{};
            exclude_files.reserve(project->excludefiles().size());
            for(auto& ef : project->excludefiles())
                exclude_files.push_back(ef.c_str());
            auto parse_result = parser->parse_files({project->path()}, exclude_files);
            diagnostic(parse_result);
            if(parse_result.has_value())
                progress_end("success");
            else
                progress_end_fail("parser error");
        } catch(std::exception& e) {
            spdlog::error("error: {}", e.what());
            notify_error(e.what());
            progress_end_fail(std::string{"error: "} + e.what());
        }
        return grpc::Status::OK;
    }

    auto LanguageServerImpl::BufferCreated(grpc::ServerContext* server_context, const Buffer* buffer, Empty* result) -> grpc::Status {
        try {
            progress_start("compiling buffer: " + buffer->path());
            auto parse_result = parser->parse_model(*buffer);
            diagnostic(parse_result);
            if(parse_result.has_value())
                progress_end("success");
            else
                progress_end_fail("parser error");
        } catch(std::exception& e) {
            spdlog::error("error: {}", e.what());
            notify_error(e.what());
            progress_end_fail(std::string("error: ") + e.what());
        }
        return grpc::Status::OK;
    }

    auto LanguageServerImpl::BufferDeleted(grpc::ServerContext* server_context, const Buffer* buffer, Empty* result) -> grpc::Status {
        notify_trace("buffer was closed: " + buffer->path());
        return grpc::Status::OK;
    }

    auto LanguageServerImpl::HandleChange(grpc::ServerContext* server_context, const Buffer* buffer, Empty* result) -> grpc::Status {
        try {
            progress_start("compiling buffer: " + buffer->path());
            auto parse_result = parser->parse_model(*buffer);
            diagnostic(parse_result);
            if(parse_result.has_value())
                progress_end("success");
            else
                progress_end_fail("parser error");
        } catch(std::exception& e) {
            spdlog::error("error: {}", e.what());
            notify_error(e.what());
            progress_end_fail(std::string("error: ") + e.what());
        }
        return grpc::Status::OK;
    }

    auto LanguageServerImpl::HandleDiff(grpc::ServerContext* server_context, const Diff* diff, Empty* result) -> grpc::Status {
        notify_warning("diffs are not supported by this language server");
        return grpc::Status::OK;
    }

    auto LanguageServerImpl::GetDiagnostics(grpc::ServerContext* server_context, const Empty* empty, grpc::ServerWriter<DiagnosticsList>* writer) -> grpc::Status {
        diagnostics_callback = [&writer, this](const DiagnosticsList& d){
            write_mutex.lock();
            writer->Write(d);
            write_mutex.unlock();
        };
        // NOTE: we now sleep forever, because we dont want to actually exit this call ever
        std::promise<void>().get_future().wait();
        return grpc::Status::OK;
    }

    auto LanguageServerImpl::GetNotifications(grpc::ServerContext* server_context, const Empty* empty, grpc::ServerWriter<Notification>* writer) -> grpc::Status {
        notifications_callback = [&writer, this](const Notification& n){ 
            write_mutex.lock();
            writer->Write(n);
            write_mutex.unlock();
        };
        // NOTE: we now sleep forever, because we dont want to actually exit this call ever
        std::promise<void>().get_future().wait();
        return grpc::Status::OK;
    }

    auto LanguageServerImpl::GetProgress(grpc::ServerContext* server_context, const Empty* empty, grpc::ServerWriter<ProgressReport>* writer) -> grpc::Status {
        progress_callback = [&writer, this](const ProgressReport& p){
            write_mutex.lock();
            writer->Write(p);
            write_mutex.unlock();
        };
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
        spdlog::trace("progress: '{}'", message);
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

    void LanguageServerImpl::diagnostic(const std::vector<Diagnostic>& diags) {
        notify_trace("diagnostics: " + std::to_string(diags.size()));
        if(!diagnostics_callback.has_value())
            return;
        DiagnosticsList l{};
        for(auto& diag : diags) {
            l.add_diagnostics()->CopyFrom(diag);
            std::stringstream ss{};
            ss << diag.message() << ": ";
            for(auto& elem : diag.affectedelements())
                ss << "[elem](" << elem << ")";
            notify_trace(ss.str());
        }
        diagnostics_callback.value()(l); 
    }

    void LanguageServerImpl::diagnostic(const plugin::parse_result& res) {
        if(res.has_value())
            diagnostic(res.value().diagnostics);
        else
            diagnostic(res.error().diagnostics);
    }
}
