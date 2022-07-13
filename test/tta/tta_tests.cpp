#include <runtime/tta/tta.h>
#include <catch2/catch_test_macros.hpp>
#include <utility>

TEST_CASE("testing_new_tta_stuff") {
    aaltitoad::ntta_t::tta_map_t component_map{};

    auto factory = aaltitoad::tta_t::graph_builder{};
    factory.add_node({"L0"});
    factory.add_node({"L1"});
    factory.add_edge("L0", "L1", {});
    factory.add_edge("L1", "L0", {});

    std::shared_ptr<aaltitoad::tta_t::graph_t> graph = std::move(factory.build_heap());
    auto tta = aaltitoad::tta_t{graph, "L0"};

    component_map["Main"] = tta;
    auto n = aaltitoad::ntta_t{{}, std::move(component_map)};
    auto changes = n.tick();
    n.apply(changes[0]);
}
