#include <ntta/tta.h>
#include <catch2/catch_test_macros.hpp>
#include <utility>
#include <ntta/ntta_builder.h>
#include <verification/forward_reachability.h>
#include <ntta/interesting_tocker.h>

SCENARIO("Trivial example", "[frs]") {
    spdlog::set_level(spdlog::level::trace);
    aaltitoad::ntta_builder builder{};
    auto n = builder
            .add_symbols({{"x", 5}})
            .add_tta("A", aaltitoad::tta_builder{builder.symbols}
                          .add_locations({"L0", "L1"})
                          .set_starting_location("L0")
                          .add_edges({{"L0", "L1", "x > -3", "x := x - 1"}, {"L1", "L0"}}))
            .build();
    n.add_tocker(std::make_shared<aaltitoad::interesting_tocker>());
    // TODO: Create a query q that should be satisfied 1 tick in
    aaltitoad::forward_reachability_searcher frs{};
    ctl::compiler compiler{n.symbols}; // TODO: ctl::compiler should support more than one symbol table environment
    auto res = compiler.parse("E F x == 0");
    if(res != 0) {
        std::cerr << "oh no" << std::endl;
        throw std::logic_error("invalid CTL expression");
    }
    auto results = frs.is_reachable(n, compiler.ast);
    // TODO: Check that FRS(n,q) returns true
    for(auto& result : results) {
        if(!result.solution.has_value()) {
            std::cout << "no solution" << std::endl;
            continue;
        }
        std::cout << "solution:\n" << result.solution.value() << std::endl;
    }
}
