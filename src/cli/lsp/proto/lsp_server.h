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
#ifndef LSP_SERVER_H
#define LSP_SERVER_H
#include <grpc/grpc.h>
#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>
#include "lsp.grpc.pb.h"
#include "lsp.pb.h"

namespace aaltitoad::lsp::proto {
    class LanguageServerImpl final : public LanguageServer::Service {
    private:
        bool running;
        int port;
        std::string semver;
    public:
        LanguageServerImpl(int port, const std::string& semver);
        void start();
        auto HandleDiff(grpc::ServerContext* server_context, const Diff* diff, Empty* result) -> grpc::Status;
        auto GetServerInfo(grpc::ServerContext* server_context, const Empty* empty, ServerInfo* result) -> grpc::Status;
        auto GetDiagnostics(grpc::ServerContext* server_context, const Empty* empty, grpc::ServerWriter<DiagnosticsList>* writer) -> grpc::Status;
        auto GetNotifications(grpc::ServerContext* server_context, const Empty* empty, grpc::ServerWriter<Notification>* writer) -> grpc::Status;
        auto GetProgress(grpc::ServerContext* server_context, const Empty* empty, grpc::ServerWriter<ProgressReport>* writer) -> grpc::Status;
    };
}

#endif // !LSP_SERVER_H
