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
#include <catch2/catch_test_macros.hpp>
#include <parser/hawk/hawk_parser.h>
#include <util/exceptions/parse_error.h>

#ifndef AALTITOAD_PROJECT_DIR
#define AALTITOAD_PROJECT_DIR "."
#endif

SCENARIO("parsing fischer-n suite", "[hawk_parser]") {
    spdlog::set_level(spdlog::level::trace);
    std::vector<std::string> folders{}, ignore_list{".*\\.ignore\\.txt"};
    GIVEN("the fischer-2 test set") {
        folders.emplace_back(AALTITOAD_PROJECT_DIR "/test/verification/fischer-suite/fischer-2");
        WHEN("parsing the network") {
            std::unique_ptr<aaltitoad::ntta_t> n{aaltitoad::hawk::load(folders, ignore_list)};
            std::cout << *n << std::endl;
            THEN("three TTAs are constructed (Main, fischer1, and fischer2)") {
                REQUIRE(n->components.size() == 3);
            }
        }
    }
    GIVEN("the fischer-5 test set") {
        folders.emplace_back(AALTITOAD_PROJECT_DIR "/test/verification/fischer-suite/fischer-5");
        WHEN("parsing the network") {
            std::unique_ptr<aaltitoad::ntta_t> n{aaltitoad::hawk::load(folders, ignore_list)};
            std::cout << *n << std::endl;
            THEN("six TTAs are constructed (fischer instances + main)") {
                REQUIRE(n->components.size() == 6);
            }
        }
    }
    GIVEN("the fischer-10 test set") {
        folders.emplace_back(AALTITOAD_PROJECT_DIR "/test/verification/fischer-suite/fischer-10");
        WHEN("parsing the network") {
            std::unique_ptr<aaltitoad::ntta_t> n{aaltitoad::hawk::load(folders, ignore_list)};
            std::cout << *n << std::endl;
            THEN("eleven TTAs are constructed (fischer instances + main)") {
                REQUIRE(n->components.size() == 11);
            }
        }
    }
}

SCENARIO("parsing failing suite", "[hawk_parser]") {
    spdlog::set_level(spdlog::level::trace);
    std::vector<std::string> folders{}, ignore_list{".*\\.ignore\\.txt"};
    GIVEN("bad-template-params") {
        folders.emplace_back(AALTITOAD_PROJECT_DIR "/test/verification/failing-suite/bad-template-params");
        THEN("parsing the network fails with a parse error") {
            REQUIRE_THROWS_AS(aaltitoad::hawk::load(folders, ignore_list), std::logic_error);
        }
    }
    GIVEN("bad-invocation-args") {
        folders.emplace_back(AALTITOAD_PROJECT_DIR "/test/verification/failing-suite/bad-invocation-args");
        THEN("parsing the network fails with a parse error") {
            REQUIRE_THROWS_AS(aaltitoad::hawk::load(folders, ignore_list), std::logic_error);
        }
    }
    GIVEN("bad-invocation-args-amount") {
        folders.emplace_back(AALTITOAD_PROJECT_DIR "/test/verification/failing-suite/bad-invocation-args-amount");
        THEN("parsing the network fails with a parse error") {
            REQUIRE_THROWS_AS(aaltitoad::hawk::load(folders, ignore_list), std::logic_error);
        }
    }
    GIVEN("bad-declarations") {
        folders.emplace_back(AALTITOAD_PROJECT_DIR "/test/verification/failing-suite/bad-declarations");
        THEN("parsing the network fails with a parse error") {
            REQUIRE_THROWS_AS(aaltitoad::hawk::load(folders, ignore_list), std::logic_error);
        }
    }
    GIVEN("bad-template-name") {
        folders.emplace_back(AALTITOAD_PROJECT_DIR "/test/verification/failing-suite/bad-template-name");
        THEN("parsing the network fails with a parse error") {
            REQUIRE_THROWS_AS(aaltitoad::hawk::load(folders, ignore_list), std::logic_error);
        }
    }
    GIVEN("bad-duplicated-locations") {
        folders.emplace_back(AALTITOAD_PROJECT_DIR "/test/verification/failing-suite/bad-duplicated-locations");
        THEN("parsing the network fails with a parse error") {
            REQUIRE_THROWS_AS(aaltitoad::hawk::load(folders, ignore_list), std::logic_error);
        }
    }
    GIVEN("bad-recursive-instantiation") {
        folders.emplace_back(AALTITOAD_PROJECT_DIR "/test/verification/failing-suite/bad-recursive-instantiation");
        THEN("parsing the network fails with a parse error") {
            REQUIRE_THROWS_AS(aaltitoad::hawk::load(folders, ignore_list), std::logic_error);
        }
    }
    GIVEN("bad-hawk-project") {
        folders.emplace_back(AALTITOAD_PROJECT_DIR "/test/verification/failing-suite/bad-hawk-project");
        THEN("parsing the network fails with a parse error") {
            REQUIRE_THROWS(aaltitoad::hawk::load(folders, ignore_list));
        }
    }
}
