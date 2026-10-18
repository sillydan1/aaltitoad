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
# Packages of aaltitoad, made with cpack. Debian packages are made from a build that was configured with the prefix of
# the package (e.g. -DCMAKE_INSTALL_PREFIX=/usr, so that the programs find their plugins):
#   cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr
#   cmake --build build
#   cd build && cpack -G DEB
# and windows installers (NSIS, with msys2/mingw) from any build, since the programs find their plugins next to them:
#   cd build && cpack -G NSIS
set(CPACK_PACKAGE_NAME aaltitoad)
set(CPACK_PACKAGE_VENDOR "Asger Gitz-Johansen")
set(CPACK_PACKAGE_CONTACT "Asger Gitz-Johansen <asger.gitz@hotmail.com>")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "verification engine and simulator for tick tock automata")
set(CPACK_PACKAGE_DESCRIPTION "aaltitoad verifies (verifier), simulates (simulator) and checks for determinism (detcheck) networks of tick tock automata, and checks and debugs them in graphedit (aaltitoad-ls). It also contains the aaltitoad library and its headers.")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/sillydan1/aaltitoad")
set(CPACK_RESOURCE_FILE_LICENSE ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE)
if(NOT WIN32)
    set(CPACK_PACKAGING_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX})
endif()

# debian / ubuntu
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
set(CPACK_DEBIAN_PACKAGE_SECTION science)
# NOTE: the dependencies (e.g. libz3) are found with dpkg-shlibdeps (from dpkg-dev)
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)

# windows: an installer that can add the programs to the PATH, with the DLLs that they need (of the compiler and of z3)
set(CPACK_PACKAGE_INSTALL_DIRECTORY aaltitoad)
set(CPACK_NSIS_PACKAGE_NAME aaltitoad)
set(CPACK_NSIS_MODIFY_PATH ON)
set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
set(CPACK_NSIS_URL_INFO_ABOUT ${CPACK_PACKAGE_HOMEPAGE_URL})
set(CPACK_NSIS_CONTACT ${CPACK_PACKAGE_CONTACT})
if(WIN32)
    get_filename_component(AALTITOAD_COMPILER_DIR ${CMAKE_CXX_COMPILER} DIRECTORY)
    install(CODE "
        if(POLICY CMP0207)
            cmake_policy(SET CMP0207 NEW) # match the (normalized) paths of the system DLLs with forward slashes
        endif()
        file(GET_RUNTIME_DEPENDENCIES
             EXECUTABLES $<TARGET_FILE:verifier> $<TARGET_FILE:simulator> $<TARGET_FILE:detcheck> $<TARGET_FILE:aaltitoad-ls>
             LIBRARIES $<TARGET_FILE:hawk_parser>
             DIRECTORIES $<TARGET_FILE_DIR:verifier> ${AALTITOAD_COMPILER_DIR}
             PRE_EXCLUDE_REGEXES \"^api-ms-\" \"^ext-ms-\"
             POST_EXCLUDE_REGEXES \".*[Ss]ystem32/.*\"
             RESOLVED_DEPENDENCIES_VAR dependencies
             UNRESOLVED_DEPENDENCIES_VAR unresolved)
        if(unresolved)
            message(WARNING \"the DLLs that were not found are not installed: \${unresolved}\")
        endif()
        file(INSTALL \${dependencies} DESTINATION \"\${CMAKE_INSTALL_PREFIX}/bin\")
    ")
endif()

include(CPack)
