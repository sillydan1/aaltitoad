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
#include "search_interrupter.h"
#include <csignal>
#include <cstdlib>
#include <spdlog/spdlog.h>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX // NOTE: the libstdc++ of mingw defines it
#define NOMINMAX
#endif
#include <windows.h>

namespace aaltitoad::windows {
    search_interrupter::search_interrupter() {
        std::signal(SIGINT, std::signal(SIGINT, SIG_IGN));
        instance = this;
        SetConsoleCtrlHandler(on_control, TRUE);
    }

    search_interrupter::~search_interrupter() {
        SetConsoleCtrlHandler(on_control, FALSE);
        instance = nullptr;
    }

    auto search_interrupter::received() const -> int {
        return signal;
    }

    auto WINAPI search_interrupter::on_control(DWORD event) -> BOOL {
        auto* interrupter = instance.load();
        if(interrupter == nullptr || (event != CTRL_C_EVENT && event != CTRL_BREAK_EVENT))
            return FALSE;
        // NOTE: the exit status of a program that SIGINT ended, as shells report it
        if(interrupter->signal != 0)
            std::_Exit(128 + SIGINT);
        spdlog::warn("ctrl-c received: interrupting the search (press it again to quit right away)");
        interrupter->signal = SIGINT;
        return TRUE;
    }
}
