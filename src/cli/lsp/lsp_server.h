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
#include "lsp.grpc.pb.h"
#include "lsp.pb.h"
#include "ntta/tta.h"
#include "plugin_system/parser.h"
#include <functional>
#include <grpc/grpc.h>
#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>
#include <optional>
#include <string>

namespace aaltitoad::lsp::proto {
    class LanguageServerImpl final : public LanguageServer::Service {
    private:
        bool running;
        int port;
        std::string semver;
        std::optional<std::function<void(const DiagnosticsList&)>> diagnostics_callback;
        std::optional<std::function<void(const Notification&)>> notifications_callback;
        std::optional<std::function<void(const ProgressReport&)>> progress_callback;
        std::vector<Diagnostic> lints;
        ntta_t network;
        std::shared_ptr<plugin::parser> parser;
        std::mutex write_mutex{};
    public:
        LanguageServerImpl(int port, const std::string& semver, const std::shared_ptr<plugin::parser>& parser);
        ~LanguageServerImpl();
        void start();

        void progress_start(const std::string& message);
        void progress(const std::string& message);
        void progress_end(const std::string& message);
        void progress_end_fail(const std::string& message);

        void notify_error(const std::string& message);
        void notify_info(const std::string& message);
        void notify_warning(const std::string& message);
        void notify_debug(const std::string& message);
        void notify_trace(const std::string& message);

        auto GetServerInfo(grpc::ServerContext* server_context, const Empty* empty, ServerInfo* result) -> grpc::Status;
        auto ProjectOpened(grpc::ServerContext* server_context, const Project* project, Empty* result) -> grpc::Status;
        auto BufferCreated(grpc::ServerContext* server_context, const Buffer* buffer, Empty* result) -> grpc::Status;
        auto BufferDeleted(grpc::ServerContext* server_context, const Buffer* buffer, Empty* result) -> grpc::Status;
        auto HandleDiff(grpc::ServerContext* server_context, const Diff* diff, Empty* result) -> grpc::Status;
        auto HandleChange(grpc::ServerContext* server_context, const Buffer* buffer, Empty* result) -> grpc::Status;
        auto GetDiagnostics(grpc::ServerContext* server_context, const Empty* empty, grpc::ServerWriter<DiagnosticsList>* writer) -> grpc::Status;
        auto GetNotifications(grpc::ServerContext* server_context, const Empty* empty, grpc::ServerWriter<Notification>* writer) -> grpc::Status;
        auto GetProgress(grpc::ServerContext* server_context, const Empty* empty, grpc::ServerWriter<ProgressReport>* writer) -> grpc::Status;
    private:
        void progress(const ProgressReportType& type, const std::string& message);
        void notify(const NotificationLevel& level, const std::string& message);
        void diagnostic(const std::vector<Diagnostic>& diags);

        void diagnostic(const plugin::parse_result& res);
    };
}

#endif // !LSP_SERVER_H
