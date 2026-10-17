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
# and windows installers (NSIS, without z3, which is installed separately) from any build in the UCRT64 environment of
# msys2, since the programs find their plugins next to them (the scanners of the dependencies need FlexLexer.h, which
# the flex of msys2 installs among the headers of msys2 itself, so it is given from a directory of its own):
#   pacman -S flex bison git mingw-w64-ucrt-x86_64-{gcc,cmake,ninja,z3,nsis}
#   mkdir flex-include && cp /usr/include/FlexLexer.h flex-include/
#   cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DFLEX_INCLUDE_DIR="$(cygpath -m "$PWD/flex-include")"
#   cmake --build build
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

# windows: an installer that can add the programs to the PATH, with the DLLs of aaltitoad (and of its dependencies that
# are built with it, e.g. spdlog) and the runtime of the compiler (libstdc++, libgcc and winpthread, with their
# licenses). The DLLs of others (e.g. z3) are not distributed. NOTE: the runtime is not linked statically, since every
# DLL would have a runtime of its own then (e.g. a std::cerr of its own, which the parser of expressions reports syntax
# errors to)
set(CPACK_PACKAGE_INSTALL_DIRECTORY aaltitoad)
set(CPACK_NSIS_PACKAGE_NAME aaltitoad)
set(CPACK_NSIS_MODIFY_PATH ON)
set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
set(CPACK_NSIS_URL_INFO_ABOUT ${CPACK_PACKAGE_HOMEPAGE_URL})
set(CPACK_NSIS_CONTACT ${CPACK_PACKAGE_CONTACT})
if(WIN32)
    get_filename_component(AALTITOAD_COMPILER_DIR ${CMAKE_CXX_COMPILER} DIRECTORY)
    string(TOLOWER "${CMAKE_BINARY_DIR}/" AALTITOAD_BUILD_DIR)
    get_filename_component(AALTITOAD_LICENSES_DIR ${AALTITOAD_COMPILER_DIR}/../share/licenses ABSOLUTE)
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
        foreach(dependency IN LISTS dependencies)
            string(TOLOWER \"\${dependency}\" path)
            get_filename_component(name \"\${path}\" NAME)
            string(FIND \"\${path}\" \"${AALTITOAD_BUILD_DIR}\" position)
            if(position EQUAL 0 OR name MATCHES \"^(libstdc\\\\+\\\\+|libgcc_s|libwinpthread)[-_].*\\\\.dll$\")
                list(APPEND installed \"\${dependency}\")
            else()
                list(APPEND others \"\${dependency}\")
            endif()
        endforeach()
        message(STATUS \"the DLLs of others are not installed: \${others}\")
        file(INSTALL \${installed} DESTINATION \"\${CMAKE_INSTALL_PREFIX}/bin\")
        foreach(license IN ITEMS gcc libwinpthread)
            if(NOT EXISTS \"${AALTITOAD_LICENSES_DIR}/\${license}\")
                message(FATAL_ERROR \"the license of \${license} is not in ${AALTITOAD_LICENSES_DIR}\")
            endif()
            file(INSTALL \"${AALTITOAD_LICENSES_DIR}/\${license}\"
                 DESTINATION \"\${CMAKE_INSTALL_PREFIX}/share/licenses\")
        endforeach()
    ")
endif()

include(CPack)
