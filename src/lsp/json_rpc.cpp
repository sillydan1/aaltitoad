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
#include "json_rpc.h"
#include <algorithm>
#include <charconv>
#include <stdexcept>

namespace aaltitoad::lsp {
    namespace {
        auto lowercase(std::string text) -> std::string {
            std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c){ return std::tolower(c); });
            return text;
        }

        auto trimmed(std::string_view text) -> std::string_view {
            while(!text.empty() && (text.front() == ' ' || text.front() == '\t'))
                text.remove_prefix(1);
            while(!text.empty() && (text.back() == ' ' || text.back() == '\t'))
                text.remove_suffix(1);
            return text;
        }
    }

    void message_reader::feed(std::string_view data) {
        buffer_.append(data);
    }

    auto message_reader::next() -> std::optional<std::string> {
        auto header_end = buffer_.find("\r\n\r\n");
        if(header_end == std::string::npos) {
            if(buffer_.size() > 64 * 1024)
                throw std::runtime_error("the header of a message is too long");
            return std::nullopt;
        }
        std::optional<std::size_t> length{};
        std::string_view headers{buffer_.data(), header_end};
        while(!headers.empty()) {
            auto line_end = headers.find("\r\n");
            auto line = headers.substr(0, line_end);
            headers.remove_prefix(line_end == std::string_view::npos ? headers.size() : line_end + 2);
            auto colon = line.find(':');
            if(colon == std::string_view::npos)
                throw std::runtime_error("a header of a message has no value: " + std::string{line});
            if(lowercase(std::string{trimmed(line.substr(0, colon))}) != "content-length")
                continue;
            auto value = trimmed(line.substr(colon + 1));
            std::size_t parsed{};
            auto [end, error] = std::from_chars(value.data(), value.data() + value.size(), parsed);
            if(error != std::errc{} || end != value.data() + value.size() || parsed > max_content_length)
                throw std::runtime_error("the content length of a message is invalid: " + std::string{value});
            length = parsed;
        }
        if(!length)
            throw std::runtime_error("a message has no content length");
        auto content_start = header_end + 4;
        if(buffer_.size() < content_start + length.value())
            return std::nullopt;
        auto content = buffer_.substr(content_start, length.value());
        buffer_.erase(0, content_start + length.value());
        return content;
    }

    auto frame(const std::string& content) -> std::string {
        return "Content-Length: " + std::to_string(content.size()) + "\r\n\r\n" + content;
    }
}
