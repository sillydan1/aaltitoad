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
#ifndef AALTITOAD_WARNINGS_H
#define AALTITOAD_WARNINGS_H
#include "lsp.pb.h"
#include "plugin_system/parser.h"
#include <unordered_map>
#include <string>
#include <vector>

namespace aaltitoad {
    enum w_t {
        overlap_idem,
        plugin_load_failed,
        unsupported_query,
        parser_warning
    };

    class warnings {
    public:
        static auto is_enabled(const w_t& warning_name) -> bool;
        static void disable_warning(const w_t& warning_name);
        static void enable_all();
        static void disable_all();
        static auto descriptions() -> std::unordered_map<w_t, std::string>;
        static void warn(const w_t& warning, const std::string& msg);
        static void warn(const w_t& warning, const std::string& msg, const std::vector<std::string>& extra_info_lines);

        static void print_diagnostic(const Diagnostic& diagnostic);
        static void print_warnings(const plugin::parse_result& parse_result);
    };
}

#endif //AALTITOAD_WARNINGS_H
