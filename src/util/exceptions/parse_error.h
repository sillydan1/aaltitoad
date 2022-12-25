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
#ifndef AALTITOAD_PARSE_ERROR_H
#define AALTITOAD_PARSE_ERROR_H
#include <aaltitoadpch.h>

class parse_error : public std::logic_error {
public:
    parse_error(const parse_error&) noexcept = default;
    explicit parse_error(const std::string& msg) : std::logic_error{msg.c_str()}, msg{msg} {}
    [[nodiscard]] const char* what() const noexcept override {
        return msg.c_str();
    }
private:
    std::string msg;
};

class aggregate_error : public std::logic_error {
public:
    aggregate_error(const aggregate_error&) noexcept = default;
    aggregate_error() : std::logic_error{"aggregate_error"}, messages{} {}
    explicit aggregate_error(std::vector<std::string>&& messages) : std::logic_error("aggregate_error"), messages{std::move(messages)} {}
    [[nodiscard]] auto message() const noexcept -> std::string {
        return join(",\n\t", messages);
    }
    [[nodiscard]] const char* what() const noexcept override {
        return (new std::string{message()})->c_str();
    }
private:
    std::vector<std::string> messages;
};

template<typename C, typename F>
void call_func_aggregate_errors(C&& collection, F&& func) {
    std::vector<std::string> errors{};
    for(auto& element : collection) {
        try {
            func(element);
        } catch (std::exception& e) {
            errors.emplace_back(e.what());
        }
    }
    if(!errors.empty())
        throw aggregate_error(std::move(errors));
}

#endif //AALTITOAD_PARSE_ERROR_H
