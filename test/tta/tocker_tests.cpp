#include <runtime/tta/interesting_tocker.h>
#include <runtime/tta/async_tocker.h>
#include <catch2/catch_test_macros.hpp>

SCENARIO("interesting tocker", "[interesting-tocker]") {
    spdlog::set_level(spdlog::level::trace);
    aaltitoad::ntta_t::tta_map_t component_map{};
    expr::symbol_table_t symbols{};
    expr::compiler compiler{symbols};
    auto compile_update = [&compiler](const std::string& updates) { compiler.trees = {}; compiler.parse(updates); return compiler.trees; };
    auto compile_guard = [&compiler](const std::string& guard) { compiler.trees = {}; compiler.parse(guard); return compiler.trees["expression_result"]; };
    auto empty_guard = compile_guard("");
    GIVEN("two external symbols and two TTA with edges guarding the respective external symbols") {
        WHEN("adding the interesting_tocker") {
            THEN("no error occurs") {

            }
        }
        WHEN("calculating tock changes") {
            THEN("three choices are available") {

            }
        }
    }
    GIVEN("two external symbols and no guards that checks them") {
        WHEN("calculating tock changes") {
            THEN("no changes are available") {

            }
        }
    }
    GIVEN("two external symbols and two TTAs guard the symbols, where selecting both in unsatisfiable") {
        WHEN("calculating tock changes") {
            THEN("two choices are available") {

            }
        }
    }
}

SCENARIO("dummy asynchronous tocker", "[dummy-async-tocker]") {
    GIVEN("a manually controllable dummy async tocker") {
        WHEN("performing tocks without finishing the async task") {
            THEN("no changes are available") {

            }
        }
        WHEN("performing tocks after finishing the async task") {
            THEN("changes are available") {

            }
        }
    }
}
