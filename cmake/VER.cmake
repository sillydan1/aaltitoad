# aaltitoad - a verification engine for tick tock automata models
#   Copyright (C) 2023 Asger Gitz-Johansen
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
execute_process(
        COMMAND git describe --long --always --dirty --exclude=* --abbrev=8
        RESULT_VARIABLE SHORT_HASH_RESULT
        OUTPUT_VARIABLE GIT_SLUG
        OUTPUT_STRIP_TRAILING_WHITESPACE)
configure_file(${CONFIG_IN_FILE} ${CONFIG_OUT_FILE})
