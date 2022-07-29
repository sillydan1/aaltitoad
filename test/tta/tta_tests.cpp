#include <runtime/tta/tta.h>
#include <catch2/catch_test_macros.hpp>
#include <utility>

TEST_CASE("testing_new_tta_stuff") {
    spdlog::set_level(spdlog::level::trace);
    aaltitoad::ntta_t::tta_map_t component_map{};
    expr::symbol_table_t declared_variables = {};
    expr::compiler compiler{declared_variables};

    auto factory = aaltitoad::tta_t::graph_builder{};
    factory.add_node({"L0"});
    factory.add_node({"L1"});
    compiler.parse("");
    auto guard = compiler.trees["expression_result"];
    expr::compiler::compiled_expr_collection_t updates{};
    factory.add_edge("L0", "L1", {.guard=guard,.updates=updates});
    factory.add_edge("L1", "L0", {.guard=guard,.updates=updates});

    aaltitoad::tta_t tta{std::move(factory.build_heap()), "L0"};

    component_map["Main"] = tta;
    auto n = aaltitoad::ntta_t{{}, component_map};
    for(int i = 0; i < 2; i++) {
        auto changes = n.tick();
        if(!changes.empty())
            n.apply(changes[0]);
    }
}

TEST_CASE("Bigger_test") {
    spdlog::set_level(spdlog::level::debug);
    aaltitoad::ntta_t::tta_map_t component_map{};
    expr::symbol_table_t declared_variables = {};
    expr::compiler compiler{declared_variables};
    compiler.parse("");
    auto empty_guard = compiler.trees["expression_result"];
    auto compile_update = [&compiler](const std::string& updates) {
        compiler.trees = {};
        compiler.parse(updates);
        return compiler.trees;
    };

    { // A
        auto factory = aaltitoad::tta_t::graph_builder{};
        factory.add_node({"L0"});
        factory.add_node({"L1"});
        factory.add_edge("L0", "L1", {.identifier="a", .guard=empty_guard, .updates=compile_update("x:=1")});
        factory.add_edge("L0", "L1", {.identifier="d", .guard=empty_guard, .updates={}});
        factory.add_edge("L0", "L1", {.identifier="e", .guard=empty_guard, .updates=compile_update("y:=1")});
        component_map["A"] = {std::move(factory.build_heap()), "L0"};
    }
    { // B
        auto factory = aaltitoad::tta_t::graph_builder{};
        factory.add_node({"L0"});
        factory.add_node({"L1"});
        factory.add_edge("L0", "L1", {.identifier="b", .guard=empty_guard, .updates=compile_update("x:=2")});
        factory.add_edge("L0", "L1", {.identifier="c", .guard=empty_guard, .updates={}});
        factory.add_edge("L0", "L1", {.identifier="f", .guard=empty_guard, .updates=compile_update("z:=1")});
        component_map["B"] = {std::move(factory.build_heap()), "L0"};
    }
    { // C
        auto factory = aaltitoad::tta_t::graph_builder{};
        factory.add_node({"L0"});
        factory.add_node({"L1"});
        factory.add_edge("L0", "L1", {.identifier="g", .guard=empty_guard, .updates={}});
        factory.add_edge("L0", "L1", {.identifier="h", .guard=empty_guard, .updates=compile_update("y:=2;z:=2")});
        component_map["C"] = {std::move(factory.build_heap()), "L0"};
    }
    { // D
        auto factory = aaltitoad::tta_t::graph_builder{};
        factory.add_node({"L0"});
        factory.add_node({"L1"});
        factory.add_edge("L0", "L1", {.identifier="i", .guard=empty_guard, .updates=compile_update("z:=3")});
        component_map["D"] = {std::move(factory.build_heap()), "L0"};
    }

    expr::symbol_table_t initial_symbols{};
    initial_symbols["x"] = 0;
    initial_symbols["y"] = 0;
    initial_symbols["z"] = 0;
    auto n = aaltitoad::ntta_t{initial_symbols, component_map};
    std::cout << std::hash<aaltitoad::ntta_t>{}(n) << "\n" << n << "===========\n";
    auto changes = n.tick();
    REQUIRE(6 == changes.size());
    n.apply(changes[0]);
    std::cout << std::hash<aaltitoad::ntta_t>{}(n) << "\n" << n << "===========\n";
}

TEST_CASE("edge_case_test") {
    spdlog::set_level(spdlog::level::debug);
    aaltitoad::ntta_t::tta_map_t component_map{};
    expr::symbol_table_t declared_variables = {};
    expr::compiler compiler{declared_variables};
    compiler.parse("");
    auto empty_guard = compiler.trees["expression_result"];
    auto compile_update = [&compiler](const std::string& updates) {
        compiler.trees = {};
        compiler.parse(updates);
        return compiler.trees;
    };

    { // A
        auto factory = aaltitoad::tta_t::graph_builder{};
        factory.add_node({"L0"});
        factory.add_node({"L1"});
        factory.add_edge("L0", "L1", {.identifier="a", .guard=empty_guard, .updates=compile_update("x:=1")});
        component_map["A"] = {std::move(factory.build_heap()), "L0"};
    }
    { // B
        auto factory = aaltitoad::tta_t::graph_builder{};
        factory.add_node({"L0"});
        factory.add_node({"L1"});
        factory.add_edge("L0", "L1", {.identifier="b", .guard=empty_guard, .updates=compile_update("x:=2")});
        component_map["B"] = {std::move(factory.build_heap()), "L0"};
    }

    auto n = aaltitoad::ntta_t{{}, component_map};
    auto changes = n.tick();
    REQUIRE(2 == changes.size());
}
