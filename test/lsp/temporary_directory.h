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
#ifndef AALTITOAD_TEST_LSP_TEMPORARY_DIRECTORY_H
#define AALTITOAD_TEST_LSP_TEMPORARY_DIRECTORY_H
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>
#include <random>

namespace aaltitoad::test {
    // A directory for a test, removed with everything in it when the test ends
    struct temporary_directory_t {
        std::filesystem::path path{};

        temporary_directory_t() {
            std::random_device random{};
            path = std::filesystem::temp_directory_path() / ("aaltitoad-test-" + std::to_string(random()) + std::to_string(random()));
            std::filesystem::create_directories(path);
        }
        temporary_directory_t(const temporary_directory_t&) = delete;
        auto operator=(const temporary_directory_t&) -> temporary_directory_t& = delete;
        ~temporary_directory_t() {
            std::error_code error{};
            std::filesystem::remove_all(path, error);
        }

        void write_text(const std::string& key, const std::string& text) const {
            auto file = path / key;
            std::filesystem::create_directories(file.parent_path());
            std::ofstream{file} << text;
        }
        void write(const std::string& key, const nlohmann::json& json) const {
            write_text(key, json.dump(4));
        }
    };
}

#endif //AALTITOAD_TEST_LSP_TEMPORARY_DIRECTORY_H
