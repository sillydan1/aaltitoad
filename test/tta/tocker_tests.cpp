#include <runtime/tta/interesting_tocker.h>
#include <runtime/tta/async_tocker.h>
#include <catch2/catch_test_macros.hpp>

// TODO: async / sync tocker scenario

SCENARIO("constructing interesting tocker", "[interesting-tocker-ctor]") {
    spdlog::set_level(spdlog::level::trace);
    aaltitoad::ntta_t::tta_map_t component_map{};
    expr::symbol_table_t symbols{};
    expr::compiler compiler{symbols};
    auto compile_update = [&compiler](const std::string& updates) { compiler.trees = {}; compiler.parse(updates); return compiler.trees; };
    auto compile_guard = [&compiler](const std::string& guard) { compiler.trees = {}; compiler.parse(guard); return compiler.trees["expression_result"]; };
    auto empty_guard = compile_guard("");
    WHEN("") {

    }
}
