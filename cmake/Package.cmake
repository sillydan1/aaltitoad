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
# Packages of aaltitoad, made with cpack from a build that was configured with the prefix of the package (e.g.
# -DCMAKE_INSTALL_PREFIX=/usr, so that the programs find their plugins):
#   cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr
#   cmake --build build
#   cd build && cpack -G DEB
set(CPACK_PACKAGE_NAME aaltitoad)
set(CPACK_PACKAGE_VENDOR "Asger Gitz-Johansen")
set(CPACK_PACKAGE_CONTACT "Asger Gitz-Johansen <asger.gitz@hotmail.com>")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "verification engine and simulator for tick tock automata")
set(CPACK_PACKAGE_DESCRIPTION "aaltitoad verifies (verifier), simulates (simulator) and checks for determinism (detcheck) networks of tick tock automata, and checks and debugs them in graphedit (aaltitoad-ls). It also contains the aaltitoad library and its headers.")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/sillydan1/aaltitoad")
set(CPACK_RESOURCE_FILE_LICENSE ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE)
set(CPACK_PACKAGING_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX})

# debian / ubuntu
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
set(CPACK_DEBIAN_PACKAGE_SECTION science)
# NOTE: the dependencies (e.g. libz3) are found with dpkg-shlibdeps (from dpkg-dev)
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)

include(CPack)
