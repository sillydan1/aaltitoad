# aaltitoad - a verification engine for tick tock automata models
#   Copyright (C) 2026 Asger Gitz-Johansen
#
#    This program is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program.  If not, see <https://www.gnu.org/licenses/>.
#
# expr finds Z3 with find_package(Z3), but some distributions (e.g. Debian and Ubuntu) do not ship the package
# configuration of Z3. This uses the configuration when there is one, and otherwise finds the library and its headers.
find_package(Z3 CONFIG QUIET)
if(Z3_FOUND)
    return()
endif()

find_path(Z3_CXX_INCLUDE_DIR NAMES z3++.h PATH_SUFFIXES z3)
find_library(Z3_LIBRARY NAMES z3 libz3)
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Z3 REQUIRED_VARS Z3_LIBRARY Z3_CXX_INCLUDE_DIR)
if(Z3_FOUND)
    set(Z3_C_INCLUDE_DIRS ${Z3_CXX_INCLUDE_DIR})
    set(Z3_CXX_INCLUDE_DIRS ${Z3_CXX_INCLUDE_DIR})
    set(Z3_LIBRARIES ${Z3_LIBRARY})
    if(NOT TARGET z3::libz3)
        add_library(z3::libz3 UNKNOWN IMPORTED)
        set_target_properties(z3::libz3 PROPERTIES
                IMPORTED_LOCATION ${Z3_LIBRARY}
                INTERFACE_INCLUDE_DIRECTORIES ${Z3_CXX_INCLUDE_DIR})
    endif()
endif()
mark_as_advanced(Z3_CXX_INCLUDE_DIR Z3_LIBRARY)
