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
#ifndef AALTITOAD_LSP_PROJECT_H
#define AALTITOAD_LSP_PROJECT_H
#include <parser/graphedit/ntta_model.h>
#include <filesystem>
#include <map>
#include <regex>

namespace aaltitoad::lsp {
    // Matches paths (relative to the project directory, separated by "/") against glob patterns, like graphedit does
    // with the excluded files of projects: "*" matches within a part of the path, "**" across parts, "?" one character,
    // "[abc]" one of the characters and "{a,b}" one of the alternatives.
    class glob_matcher {
    public:
        void add(const std::string& pattern);
        // Add the patterns of an ignore file (one per line, "#" starts a comment), if it exists
        void add_file(const std::filesystem::path& file);
        auto matches(const std::string& path) const -> bool;
        // The regular expression of a glob pattern (anchored)
        static auto to_regex(const std::string& pattern) -> std::string;

    private:
        std::vector<std::regex> patterns_{};
    };

    // The NTTA models of a graphedit project, read from the model files in the project directory (and its
    // subdirectories). Hidden and excluded files do not belong to the project. Files are read again when they change.
    class project_files {
    public:
        project_files(std::filesystem::path directory, const std::vector<std::string>& exclude_patterns);

        auto directory() const -> const std::filesystem::path&;
        // Read the files that changed since the last scan (and forget the files that are gone)
        void scan();
        // The models, by key (the path relative to the project directory)
        auto models() const -> std::map<std::string, graphedit::ntta_model_t>;
        // The files of NTTA models that cannot be read, by key, with the reason
        auto unreadable() const -> std::map<std::string, std::string>;

    private:
        struct file_t {
            std::filesystem::file_time_type time{};
            std::uintmax_t size{0};
            std::optional<graphedit::ntta_model_t> model{};
            std::string problem{};
        };
        auto read(const std::filesystem::path& path, const std::string& key) const -> file_t;

        std::filesystem::path directory_;
        glob_matcher excluded_{};
        std::map<std::string, file_t> files_{};
    };
}

#endif //AALTITOAD_LSP_PROJECT_H
