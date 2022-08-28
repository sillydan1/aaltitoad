#ifndef AALTITOAD_FORWARD_REACHABILITY_H
#define AALTITOAD_FORWARD_REACHABILITY_H
#include "ctl_sat.h"
#include "ntta/tta.h"
#include "traceable_multimap.h"
#include <ctl_compiler.h>
#include <vector>
#include <utility>

namespace aaltitoad {
    class forward_reachability_searcher {
    public:
        using solution_t = traceable_multimap<ntta_t>::iterator_t;
        using compiled_query_t = ctl::compiler::compiled_expr_t;
        struct query_solution_t {
            compiled_query_t query;
            std::optional<solution_t> solution;
            query_solution_t(compiled_query_t query) : query{std::move(query)}, solution{} {}
        };
        using solutions_t = std::vector<query_solution_t>;
        forward_reachability_searcher() = default;
        auto is_reachable(const ntta_t& s0, const compiled_query_t& q) -> solutions_t;
        auto is_reachable(const ntta_t& s0, const std::vector<compiled_query_t>& q) -> solutions_t;

    private:
        traceable_multimap<ntta_t> W{}, P{};
        solutions_t solutions{};

        static auto empty_solution_set(const std::vector<compiled_query_t>& q) -> solutions_t;
        auto check_satisfactions(const solution_t& s) -> bool;
        auto count_solutions() -> size_t;
    };
}
auto operator<<(std::ostream& o, const aaltitoad::forward_reachability_searcher::solution_t& s) -> std::ostream&;

#endif //AALTITOAD_FORWARD_REACHABILITY_H
