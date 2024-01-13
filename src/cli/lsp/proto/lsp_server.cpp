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
#include "lsp_server.h"
#include <spdlog/spdlog.h>
#include <grpcpp/support/status.h>

namespace aaltitoad::lsp::proto {
    LanguageServerImpl::LanguageServerImpl(int port, const std::string& semver) : running{false}, port{port}, semver{semver} {

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
        std::stringstream ss{};
        for(auto x : diff->vertexadditions())
            ss << "+v: {}" << x.jsonencoding();
        for(auto x : diff->edgeadditions())
            ss << "+e: {}" << x.jsonencoding();
        for(auto x : diff->vertexdeletions())
            ss << "-v: {}" << x.jsonencoding();
        for(auto x : diff->edgedeletions())
            ss << "-e: {}" << x.jsonencoding();
        spdlog::info("new diff: \n {}", ss.str());
        return grpc::Status::OK;
    }

    auto LanguageServerImpl::GetServerInfo(grpc::ServerContext* server_context, const Empty* empty, ServerInfo* result) -> grpc::Status {
        result->set_name("aaltitoad-ls");
        result->set_language("hawk");
        result->set_semanticversion(semver);
        return grpc::Status::OK;
    }

    auto LanguageServerImpl::GetDiagnostics(grpc::ServerContext* server_context, const Empty* empty, grpc::ServerWriter<DiagnosticsList>* writer) -> grpc::Status {
        spdlog::warn("diagnostics not implemented yet");
        return grpc::Status::OK; // NOTE: Only return when exiting
    }

    auto LanguageServerImpl::GetNotifications(grpc::ServerContext* server_context, const Empty* empty, grpc::ServerWriter<Notification>* writer) -> grpc::Status {
        spdlog::warn("notifications not implemented yet");
        return grpc::Status::OK; // NOTE: Only return when exiting
    }

    auto LanguageServerImpl::GetProgress(grpc::ServerContext* server_context, const Empty* empty, grpc::ServerWriter<ProgressReport>* writer) -> grpc::Status {
        spdlog::warn("progress reports not implemented yet");
        return grpc::Status::OK; // NOTE: Only return when exiting
    }
}
