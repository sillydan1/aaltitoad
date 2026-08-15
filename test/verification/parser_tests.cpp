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
#include "ntta/tta.h"
#include <catch2/catch_test_macros.hpp>
#include <parser/hawk/huppaal/parser.h>
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
            auto parse_result = aaltitoad::hawk::huppaal::parser{}.parse_files(folders, ignore_list);
            REQUIRE(parse_result.has_value());
            CHECK(parse_result.value().diagnostics.size() == 1); // TODO: The success state should not result in diagnostics
            auto n = std::move(parse_result.value().ntta);
            std::cout << n << std::endl;
            THEN("three TTAs are constructed (Main, fischer1, and fischer2)") {
                REQUIRE(n.components.size() == 3);
            }
        }
    }
    GIVEN("the fischer-5 test set") {
        folders.emplace_back(AALTITOAD_PROJECT_DIR "/test/verification/fischer-suite/fischer-5");
        WHEN("parsing the network") {
            auto parse_result = aaltitoad::hawk::huppaal::parser{}.parse_files(folders, ignore_list);
            REQUIRE(parse_result.has_value());
            CHECK(parse_result.value().diagnostics.size() == 4); // TODO: The success state should not result in diagnostics
            auto n = std::move(parse_result.value().ntta);
            std::cout << n << std::endl;
            THEN("six TTAs are constructed (fischer instances + main)") {
                REQUIRE(n.components.size() == 6);
            }
        }
    }
    GIVEN("the fischer-10 test set") {
        folders.emplace_back(AALTITOAD_PROJECT_DIR "/test/verification/fischer-suite/fischer-10");
        WHEN("parsing the network") {
            auto parse_result = aaltitoad::hawk::huppaal::parser{}.parse_files(folders, ignore_list);
            CHECK(parse_result.value().diagnostics.size() == 9); // TODO: The success state should not result in diagnostics
            REQUIRE(parse_result.has_value());
            auto n = std::move(parse_result.value().ntta);
            std::cout << n << std::endl;
            THEN("eleven TTAs are constructed (fischer instances + main)") {
                REQUIRE(n.components.size() == 11);
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
            auto res = aaltitoad::hawk::huppaal::parser{}.parse_files(folders, ignore_list);
            REQUIRE(!res.has_value());
            REQUIRE(res.error().diagnostics.size() == 2); // TODO: We should prune duplicates
            CHECK(res.error().diagnostics[0].message() == "Template parameter names must be unique");
            CHECK(res.error().diagnostics[1].message() == "Template parameter names must be unique");
        }
    }
    GIVEN("bad-invocation-args") {
        folders.emplace_back(AALTITOAD_PROJECT_DIR "/test/verification/failing-suite/bad-invocation-args");
        THEN("parsing the network fails with a parse error") {
            REQUIRE_THROWS_AS(aaltitoad::hawk::huppaal::parser{}.parse_files(folders, ignore_list), std::logic_error);
        }
    }
    GIVEN("bad-invocation-args-amount") {
        folders.emplace_back(AALTITOAD_PROJECT_DIR "/test/verification/failing-suite/bad-invocation-args-amount");
        THEN("parsing the network fails with a parse error") {
            auto res = aaltitoad::hawk::huppaal::parser{}.parse_files(folders, ignore_list);
            REQUIRE(!res.has_value());
            REQUIRE(res.error().diagnostics.size() == 2); // TODO: We should prune duplicates
            CHECK(res.error().diagnostics[0].message() == "Provided arguments (3) does not match parameters (2)");
            CHECK(res.error().diagnostics[1].message() == "Provided arguments (3) does not match parameters (2)");
        }
    }
    GIVEN("bad-declarations") {
        folders.emplace_back(AALTITOAD_PROJECT_DIR "/test/verification/failing-suite/bad-declarations");
        THEN("parsing the network fails with a parse error") {
            // TODO: This is a bit weird, that _sometimes_ the parser throws and exception, and _sometimes_ it doesnt...
            REQUIRE_THROWS_AS(aaltitoad::hawk::huppaal::parser{}.parse_files(folders, ignore_list), std::logic_error);
        }
    }
    GIVEN("bad-template-name") {
        folders.emplace_back(AALTITOAD_PROJECT_DIR "/test/verification/failing-suite/bad-template-name");
        THEN("parsing the network fails with a parse error") {
            REQUIRE_THROWS_AS(aaltitoad::hawk::huppaal::parser{}.parse_files(folders, ignore_list), std::logic_error);
        }
    }
    GIVEN("bad-duplicated-locations") {
        folders.emplace_back(AALTITOAD_PROJECT_DIR "/test/verification/failing-suite/bad-duplicated-locations");
        THEN("parsing the network fails with a parse error") {
            auto res = aaltitoad::hawk::huppaal::parser{}.parse_files(folders, ignore_list);
            REQUIRE(!res.has_value());
            REQUIRE(res.error().diagnostics.size() == 1);
            CHECK(res.error().diagnostics[0].message() == "Locations with same name is not allowed");
        }
    }
    GIVEN("bad-recursive-instantiation") {
        folders.emplace_back(AALTITOAD_PROJECT_DIR "/test/verification/failing-suite/bad-recursive-instantiation");
        THEN("parsing the network fails with a parse error") {
            auto res = aaltitoad::hawk::huppaal::parser{}.parse_files(folders, ignore_list);
            REQUIRE(!res.has_value());
            REQUIRE(res.error().diagnostics.size() == 1);
            CHECK(res.error().diagnostics[0].message().find("There are loops in the instantiation tree") != std::string::npos);
        }
    }
    GIVEN("bad-hawk-project") {
        folders.emplace_back(AALTITOAD_PROJECT_DIR "/test/verification/failing-suite/bad-hawk-project");
        THEN("parsing the network fails with a parse error") {
            REQUIRE_THROWS(aaltitoad::hawk::huppaal::parser{}.parse_files(folders, ignore_list));
        }
    }
}
