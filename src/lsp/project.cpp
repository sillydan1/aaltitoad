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
#include "project.h"
#include <spdlog/spdlog.h>
#include <fstream>

namespace aaltitoad::lsp {
    namespace {
        auto escape(char c) -> std::string {
            if(std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '/' || c == '-' || c == ' ')
                return std::string{c};
            return std::string{"\\"} + c;
        }

        // Translates a glob pattern into an (unanchored) regular expression, as graphedit does (glob_matcher). For example:
        //   *.json          ->  [^/]*\.json
        //   **/generated/*  ->  (?:.*/)?generated/[^/]*
        //   build/**        ->  build/.*
        //   model-?.json    ->  model-[^/]\.json
        //   [!m]ain.json    ->  [^m]ain\.json
        //   {a,b}/x.json    ->  (?:a|b)/x\.json
        auto glob_body(const std::string& glob) -> std::string {
            std::string result{};
            auto n = glob.size();
            auto brace_depth = 0;
            for(std::size_t i = 0; i < n; i++) {
                auto c = glob[i];
                if(c == '*') {
                    if(i + 1 < n && glob[i + 1] == '*') {
                        auto at_segment_start = i == 0 || glob[i - 1] == '/';
                        auto followed_by_slash = i + 2 < n && glob[i + 2] == '/';
                        if(at_segment_start && followed_by_slash) {
                            result += "(?:.*/)?";
                            i += 2;
                        } else {
                            result += ".*";
                            i += 1;
                        }
                    } else {
                        result += "[^/]*";
                    }
                } else if(c == '?') {
                    result += "[^/]";
                } else if(c == '[') {
                    auto j = i + 1;
                    if(j < n && (glob[j] == '!' || glob[j] == '^'))
                        j++;
                    if(j < n && glob[j] == ']')
                        j++;
                    while(j < n && glob[j] != ']')
                        j++;
                    if(j >= n) {
                        result += "\\[";
                        continue;
                    }
                    auto set = glob.substr(i + 1, j - i - 1);
                    std::string escaped{};
                    for(auto s : set)
                        escaped += s == '\\' ? std::string{"\\\\"} : std::string{s};
                    if(!escaped.empty() && escaped.front() == '!')
                        escaped.front() = '^';
                    result += "[" + escaped + "]";
                    i = j;
                } else if(c == '{') {
                    brace_depth++;
                    result += "(?:";
                } else if(c == '}' && brace_depth > 0) {
                    brace_depth--;
                    result += ")";
                } else if(c == ',' && brace_depth > 0) {
                    result += "|";
                } else if(c == '\\' && i + 1 < n) {
                    result += escape(glob[i + 1]);
                    i++;
                } else {
                    result += escape(c);
                }
            }
            for(; brace_depth > 0; brace_depth--)
                result += ")";
            return result;
        }

        auto normalized(std::string pattern) -> std::string {
            auto begin = pattern.find_first_not_of(" \t\r\n");
            auto end = pattern.find_last_not_of(" \t\r\n");
            pattern = begin == std::string::npos ? std::string{} : pattern.substr(begin, end - begin + 1);
            while(!pattern.empty() && pattern.front() == '/')
                pattern.erase(0, 1);
            while(!pattern.empty() && pattern.back() == '/')
                pattern.pop_back();
            return pattern;
        }

        auto is_hidden(const std::filesystem::path& path) -> bool {
            auto name = path.filename().string();
            return !name.empty() && name.front() == '.';
        }
    }

    auto glob_matcher::to_regex(const std::string& pattern) -> std::string {
        return "^" + glob_body(normalized(pattern)) + "$";
    }

    void glob_matcher::add(const std::string& pattern) {
        auto clean = normalized(pattern);
        if(clean.empty())
            return;
        try {
            patterns_.emplace_back(to_regex(clean));
        } catch(const std::regex_error& e) {
            spdlog::warn("ignoring the invalid pattern of excluded files '{}': {}", pattern, e.what());
        }
    }

    void glob_matcher::add_file(const std::filesystem::path& file) {
        std::ifstream input{file};
        std::string line{};
        while(std::getline(input, line)) {
            auto clean = normalized(line);
            if(!clean.empty() && clean.front() != '#')
                add(clean);
        }
    }

    auto glob_matcher::matches(const std::string& path) const -> bool {
        return std::any_of(patterns_.begin(), patterns_.end(), [&path](const std::regex& r){ return std::regex_match(path, r); });
    }

    project_files::project_files(std::filesystem::path directory, const std::vector<std::string>& exclude_patterns)
            : directory_{std::move(directory)} {
        for(auto& pattern : exclude_patterns)
            excluded_.add(pattern);
        excluded_.add_file(directory_ / ".graphedit.ignore");
    }

    auto project_files::directory() const -> const std::filesystem::path& {
        return directory_;
    }

    void project_files::scan() {
        std::map<std::string, file_t> files{};
        std::error_code error{};
        std::filesystem::recursive_directory_iterator it{directory_, std::filesystem::directory_options::skip_permission_denied, error};
        if(error) {
            spdlog::warn("cannot read the project directory '{}': {}", directory_.string(), error.message());
            files_.clear();
            return;
        }
        for(; it != std::filesystem::recursive_directory_iterator{}; it.increment(error)) {
            if(error) {
                spdlog::warn("cannot read the project directory '{}': {}", directory_.string(), error.message());
                break;
            }
            const auto& entry = *it;
            if(is_hidden(entry.path())) {
                if(entry.is_directory(error))
                    it.disable_recursion_pending();
                continue;
            }
            if(!entry.is_regular_file(error) || entry.path().extension() != ".json")
                continue;
            auto key = std::filesystem::relative(entry.path(), directory_, error).generic_string();
            if(error || excluded_.matches(key))
                continue;
            auto time = entry.last_write_time(error);
            auto size = entry.file_size(error);
            auto known = files_.find(key);
            if(known != files_.end() && known->second.time == time && known->second.size == size) {
                files[key] = std::move(known->second);
                continue;
            }
            auto file = read(entry.path(), key);
            file.time = time;
            file.size = size;
            files[key] = std::move(file);
        }
        files_ = std::move(files);
    }

    auto project_files::read(const std::filesystem::path& path, const std::string& key) const -> file_t {
        file_t result{};
        nlohmann::json document{};
        try {
            std::ifstream input{path};
            document = nlohmann::json::parse(input);
        } catch(const std::exception& e) {
            spdlog::debug("{} is not a json document: {}", key, e.what());
            return result;
        }
        try {
            result.model = graphedit::read_model_file(key, document);
            if(!result.model && graphedit::is_legacy_model_file(document))
                result.problem = "The model is in the format of graphedit 1.x, which aaltitoad cannot read: open it in "
                                 "graphedit and save it to convert it";
        } catch(const std::exception& e) {
            result.problem = std::string{"aaltitoad cannot read the model: "} + e.what();
        }
        return result;
    }

    auto project_files::models() const -> std::map<std::string, graphedit::ntta_model_t> {
        std::map<std::string, graphedit::ntta_model_t> result{};
        for(auto& [key, file] : files_)
            if(file.model)
                result.emplace(key, file.model.value());
        return result;
    }

    auto project_files::unreadable() const -> std::map<std::string, std::string> {
        std::map<std::string, std::string> result{};
        for(auto& [key, file] : files_)
            if(!file.problem.empty())
                result.emplace(key, file.problem);
        return result;
    }
}
