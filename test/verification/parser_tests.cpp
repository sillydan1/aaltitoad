#include <catch2/catch_test_macros.hpp>
#include <parser/huppaal_parser.h>

SCENARIO("parsing fischer-n suite", "[huppaal_parser]") {
    spdlog::set_level(spdlog::level::trace);
    std::vector<std::string> folders{}, ignore_list{};
    GIVEN("the fischer-2 test set") {
#ifdef AALTITOAD_PROJECT_DIR
        folders.emplace_back(AALTITOAD_PROJECT_DIR "/test/verification/fischer-2");
        // TODO: Add support for regex file-ignore
        ignore_list.emplace_back(AALTITOAD_PROJECT_DIR "/test/verification/fischer-2/expected_results.ignore.txt");
#else
        folders.emplace_back("test/verification/fischer-2");
#endif
        WHEN("parsing the network") {
            std::unique_ptr<aaltitoad::ntta_t> n{aaltitoad::huppaal::load(folders, ignore_list)};
            std::cout << *n << std::endl;
            THEN("three TTAs are constructed (Main, fischer1, and fischer2)") {
                REQUIRE(n->components.size() == 3);
            }
        }
    }
    GIVEN("the fischer-5 test set") {
#ifdef AALTITOAD_PROJECT_DIR
        folders.emplace_back(AALTITOAD_PROJECT_DIR "/test/verification/fischer-5");
        // TODO: Add support for regex file-ignore
        ignore_list.emplace_back(AALTITOAD_PROJECT_DIR "/test/verification/fischer-5/expected_results.ignore.txt");
#else
        folders.emplace_back("test/verification/fischer-2");
#endif
        WHEN("parsing the network") {
            std::unique_ptr<aaltitoad::ntta_t> n{aaltitoad::huppaal::load(folders, ignore_list)};
            std::cout << *n << std::endl;
            THEN("six TTAs are constructed (fischer instances + main)") {
                REQUIRE(n->components.size() == 6);
            }
        }
    }
    GIVEN("the fischer-10 test set") {
#ifdef AALTITOAD_PROJECT_DIR
        folders.emplace_back(AALTITOAD_PROJECT_DIR "/test/verification/fischer-10");
        // TODO: Add support for regex file-ignore
        ignore_list.emplace_back(AALTITOAD_PROJECT_DIR "/test/verification/fischer-10/expected_results.ignore.txt");
#else
        folders.emplace_back("test/verification/fischer-2");
#endif
        WHEN("parsing the network") {
            std::unique_ptr<aaltitoad::ntta_t> n{aaltitoad::huppaal::load(folders, ignore_list)};
            std::cout << *n << std::endl;
            THEN("eleven TTAs are constructed (fischer instances + main)") {
                REQUIRE(n->components.size() == 11);
            }
        }
    }
}
