#include <catch2/catch_test_macros.hpp>
#include <parser/hawk/hawk_parser.h>
#ifndef AALTITOAD_PROJECT_DIR
#define AALTITOAD_PROJECT_DIR "."
#endif

SCENARIO("parsing fischer-n suite", "[huppaal_parser]") {
    spdlog::set_level(spdlog::level::trace);
    std::vector<std::string> folders{}, ignore_list{".*\\.ignore\\.txt"};
    GIVEN("the fischer-2 test set") {
        folders.emplace_back(AALTITOAD_PROJECT_DIR "/test/verification/fischer-2");
        WHEN("parsing the network") {
            std::unique_ptr<aaltitoad::ntta_t> n{aaltitoad::huppaal::load(folders, ignore_list)};
            std::cout << *n << std::endl;
            THEN("three TTAs are constructed (Main, fischer1, and fischer2)") {
                REQUIRE(n->components.size() == 3);
            }
        }
    }
    GIVEN("the fischer-5 test set") {
        folders.emplace_back(AALTITOAD_PROJECT_DIR "/test/verification/fischer-5");
        WHEN("parsing the network") {
            std::unique_ptr<aaltitoad::ntta_t> n{aaltitoad::huppaal::load(folders, ignore_list)};
            std::cout << *n << std::endl;
            THEN("six TTAs are constructed (fischer instances + main)") {
                REQUIRE(n->components.size() == 6);
            }
        }
    }
    GIVEN("the fischer-10 test set") {
        folders.emplace_back(AALTITOAD_PROJECT_DIR "/test/verification/fischer-10");
        WHEN("parsing the network") {
            std::unique_ptr<aaltitoad::ntta_t> n{aaltitoad::huppaal::load(folders, ignore_list)};
            std::cout << *n << std::endl;
            THEN("eleven TTAs are constructed (fischer instances + main)") {
                REQUIRE(n->components.size() == 11);
            }
        }
    }
}
