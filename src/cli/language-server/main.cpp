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
#include <condition_variable>
#include <cstring>
#include <memory>
#include <mutex>
#include <thread>
#include <unistd.h>
#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif
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

    // What arrived on the standard input, and whether it ended
    struct input_t {
        std::mutex mutex{};
        std::condition_variable arrived{};
        std::string data{};
        input_status status{input_status::open};
    };

    // Read the standard input until it ends. This runs on a thread of its own, so that the server can wait for messages
    // and for its work at the same time (windows cannot poll pipes)
    void read_input(const std::shared_ptr<input_t>& input) {
        std::array<char, 64 * 1024> buffer;
        while(true) {
            auto n = ::read(STDIN_FILENO, buffer.data(), static_cast<unsigned int>(buffer.size()));
            if(n < 0 && errno == EINTR)
                continue;
            std::lock_guard lock{input->mutex};
            if(n > 0)
                input->data.append(buffer.data(), static_cast<std::size_t>(n));
            else if(n == 0) {
                spdlog::info("the standard input was closed");
                input->status = input_status::closed;
            } else {
                spdlog::error("cannot read messages: {}", std::strerror(errno));
                input->status = input_status::failed;
            }
            input->arrived.notify_one();
            if(n <= 0)
                return;
        }
    }

    // Wait until messages arrive (or the input ends), or the server has work to do, and give what arrived to the reader
    auto wait_for_input(const aaltitoad::lsp::language_server& server, input_t& input, aaltitoad::lsp::message_reader& reader) -> input_status {
        std::unique_lock lock{input.mutex};
        auto arrived = [&input]{ return !input.data.empty() || input.status != input_status::open; };
        if(auto due = server.next_work())
            input.arrived.wait_until(lock, due.value(), arrived);
        else
            input.arrived.wait(lock, arrived);
        if(!input.data.empty()) {
            reader.feed(input.data);
            input.data.clear();
        }
        return input.status;
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
    auto receive_messages(aaltitoad::lsp::language_server& server, input_t& input, aaltitoad::lsp::message_reader& reader) -> input_status {
        auto status = wait_for_input(server, input, reader);
        if(!handle_messages(server, reader))
            return input_status::failed;
        return status;
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
#ifdef _WIN32
    // NOTE: the messages are framed by their lengths in bytes, so the line endings must not be translated
    _setmode(_fileno(stdin), _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);
#endif

    auto broken_pipe = false;
    aaltitoad::lsp::language_server server{[&broken_pipe](const std::string& message) {
        if(!broken_pipe && !write_all(aaltitoad::lsp::frame(message))) {
            spdlog::error("cannot write to the standard output: {}", std::strerror(errno));
            broken_pipe = true;
        }
    }, PROJECT_VER};
    // NOTE: the reader thread is not joined, since a read cannot be interrupted (the program ends while it reads)
    auto input = std::make_shared<input_t>();
    std::thread{read_input, input}.detach();
    aaltitoad::lsp::message_reader reader{};
    while(!server.should_exit() && !broken_pipe) {
        auto status = receive_messages(server, *input, reader);
        if(status == input_status::failed)
            return 1;
        if(status == input_status::closed)
            break;
        if(!server.should_exit())
            server.work();
    }
    return server.exit_code();
}
