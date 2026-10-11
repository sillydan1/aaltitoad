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
#include <cli/cli_common.h>
#include <config.h>
#include <lsp/json_rpc.h>
#include <lsp/language_server.h>
#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/spdlog.h>
#include <algorithm>
#include <array>
#include <cerrno>
#include <cstring>
#include <poll.h>
#include <unistd.h>
#include "cli_options.h"

// aaltitoad-ls: the language server of aaltitoad for graphedit. It speaks the graph language server protocol of
// graphedit (JSON-RPC over the standard input and output) for NTTA models, and debugs them. Its log goes to the standard
// error, which graphedit writes to its log.
namespace {
    // NOTE: the standard output is the channel of the protocol, so nothing else may be written to it
    void log_to_stderr(int verbosity) {
        spdlog::set_default_logger(spdlog::stderr_logger_mt("aaltitoad-ls"));
        spdlog::set_level(static_cast<spdlog::level::level_enum>(std::clamp(SPDLOG_LEVEL_OFF - verbosity, 0, SPDLOG_LEVEL_OFF)));
    }

    // Write all of the data to the standard output
    auto write_all(const std::string& data) -> bool {
        std::size_t written = 0;
        while(written < data.size()) {
            auto n = ::write(STDOUT_FILENO, data.data() + written, data.size() - written);
            if(n < 0 && errno == EINTR)
                continue;
            if(n <= 0)
                return false;
            written += static_cast<std::size_t>(n);
        }
        return true;
    }

    enum class input_status { open, closed, failed };

    // How long to wait for messages before the server has work to do (-1: until a message arrives)
    auto wait_timeout(const aaltitoad::lsp::language_server& server) -> int {
        auto due = server.next_work();
        if(!due)
            return -1;
        auto wait = std::chrono::duration_cast<std::chrono::milliseconds>(due.value() - std::chrono::steady_clock::now()).count();
        return static_cast<int>(std::clamp<long long>(wait, 0, 60000));
    }

    // Wait until the standard input has data or the timeout passes. Returns whether there is data to read
    auto wait_for_input(int timeout) -> std::optional<bool> {
        pollfd input{STDIN_FILENO, POLLIN, 0};
        auto ready = ::poll(&input, 1, timeout);
        if(ready < 0 && errno != EINTR) {
            spdlog::error("cannot wait for messages: {}", std::strerror(errno));
            return std::nullopt;
        }
        return ready > 0;
    }

    // Read what the standard input has into the reader
    auto read_input(aaltitoad::lsp::message_reader& reader) -> input_status {
        std::array<char, 64 * 1024> buffer;
        auto n = ::read(STDIN_FILENO, buffer.data(), buffer.size());
        if(n == 0) {
            spdlog::info("the standard input was closed");
            return input_status::closed;
        }
        if(n < 0 && errno != EINTR && errno != EAGAIN) {
            spdlog::error("cannot read messages: {}", std::strerror(errno));
            return input_status::failed;
        }
        if(n > 0)
            reader.feed({buffer.data(), static_cast<std::size_t>(n)});
        return input_status::open;
    }

    // Handle the messages that arrived completely. Returns false if the messages cannot be read any further
    auto handle_messages(aaltitoad::lsp::language_server& server, aaltitoad::lsp::message_reader& reader) -> bool {
        try {
            while(!server.should_exit()) {
                auto message = reader.next();
                if(!message)
                    break;
                server.handle(message.value());
            }
            return true;
        } catch(const std::exception& e) {
            spdlog::critical("cannot read the messages any further: {}", e.what());
            return false;
        }
    }

    // Wait for messages (until the server has work to do) and handle those that arrive
    auto receive_messages(aaltitoad::lsp::language_server& server, aaltitoad::lsp::message_reader& reader) -> input_status {
        auto ready = wait_for_input(wait_timeout(server));
        if(!ready)
            return input_status::failed;
        if(!ready.value())
            return input_status::open;
        auto status = read_input(reader);
        if(status != input_status::open)
            return status;
        return handle_messages(server, reader) ? input_status::open : input_status::failed;
    }
}

int main(int argc, char** argv) {
    auto options = get_options();
    auto cli_arguments = get_arguments(options, argc, argv);
    if(cli_arguments["version"])
        return print_version();
    if(cli_arguments["help"])
        return print_help(argv[0], options, "[OPTIONS]");
    log_to_stderr(cli_arguments["verbosity"].as_integer_or_default(3));

    auto broken_pipe = false;
    aaltitoad::lsp::language_server server{[&broken_pipe](const std::string& message) {
        if(!broken_pipe && !write_all(aaltitoad::lsp::frame(message))) {
            spdlog::error("cannot write to the standard output: {}", std::strerror(errno));
            broken_pipe = true;
        }
    }, PROJECT_VER};
    aaltitoad::lsp::message_reader reader{};
    while(!server.should_exit() && !broken_pipe) {
        auto status = receive_messages(server, reader);
        if(status == input_status::failed)
            return 1;
        if(status == input_status::closed)
            break;
        if(!server.should_exit())
            server.work();
    }
    return server.exit_code();
}
