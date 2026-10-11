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
#ifndef AALTITOAD_LSP_JSON_RPC_H
#define AALTITOAD_LSP_JSON_RPC_H
#include <optional>
#include <string>
#include <string_view>

// The framing of JSON-RPC messages of the language server protocol: "Content-Length: <bytes>\r\n\r\n<content>"
namespace aaltitoad::lsp {
    // The error codes of JSON-RPC
    inline constexpr int parse_error = -32700;
    inline constexpr int invalid_request = -32600;
    inline constexpr int method_not_found = -32601;
    inline constexpr int invalid_params = -32602;
    inline constexpr int internal_error = -32603;
    inline constexpr int server_not_initialized = -32002;

    // Splits a stream of bytes into messages
    class message_reader {
    public:
        // The largest message that is accepted (graphedit sends whole models)
        static constexpr std::size_t max_content_length = 512 * 1024 * 1024;

        void feed(std::string_view data);
        // The content of the next complete message, if one arrived. Throws std::runtime_error if the stream is not
        // framed correctly (the stream cannot be read any further then).
        auto next() -> std::optional<std::string>;

    private:
        std::string buffer_{};
    };

    // A message framed for sending
    auto frame(const std::string& content) -> std::string;
}

#endif //AALTITOAD_LSP_JSON_RPC_H
