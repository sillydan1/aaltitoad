#include <runtime/tta/tta.h>
#include <catch2/catch_test_macros.hpp>
#include <utility>

TEST_CASE("testing_new_tta_stuff") {
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
    std::cout << n << std::endl;
    for(int i = 0; i < 2; i++) {
        auto changes = n.tick();
        if(!changes.empty())
            n.apply(changes[0]);
        std::cout << n << std::endl;
    }
}
