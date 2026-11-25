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
#ifndef AALTITOAD_WINDOWS_SEARCH_INTERRUPTER_H
#define AALTITOAD_WINDOWS_SEARCH_INTERRUPTER_H
#include <atomic>

namespace aaltitoad::windows {
    // Interrupts the search on ctrl-c (or ctrl-break) while it exists, so that the results found so far are printed. A
    // second one ends the program right away. Console control handlers are called from the last one that was added, so
    // the handler of the C runtime (which calls the SIGINT handlers, e.g. the one of z3 that cancels only the query that
    // it is solving) is added first, and this one after it
    class search_interrupter {
    public:
        search_interrupter();
        search_interrupter(const search_interrupter&) = delete;
        auto operator=(const search_interrupter&) -> search_interrupter& = delete;
        ~search_interrupter();
        // the signal that interrupted the search (0 if it was not interrupted)
        auto received() const -> int;

    private:
        // NOTE: a console control handler (a PHANDLER_ROUTINE, i.e. BOOL WINAPI (DWORD)), declared without windows.h
        static auto __stdcall on_control(unsigned long event) -> int;

        static inline std::atomic<search_interrupter*> instance{nullptr};
        std::atomic<int> signal{0};
    };
}

#endif //AALTITOAD_WINDOWS_SEARCH_INTERRUPTER_H
