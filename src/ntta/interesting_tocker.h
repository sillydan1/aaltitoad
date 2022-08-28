#ifndef AALTITOAD_INTERESTING_TOCKER_H
#define AALTITOAD_INTERESTING_TOCKER_H
#include "tta.h"
#include <drivers/z3_driver.h>
#include <unordered_map>

namespace aaltitoad {
    class interesting_tocker : public tocker_t {
    public:
        [[nodiscard]] auto tock(const ntta_t& state) -> std::vector<expr::symbol_table_t> override;
        [[nodiscard]] auto get_name() -> std::string override;
        ~interesting_tocker() override = default;
    private:
        [[nodiscard]] auto contains_external_variables(const expr::syntax_tree_t& tree, const expr::symbol_table_t& symbols) const -> bool;
        static auto find_solution(expr::z3_driver& d, const ya::combiner_iterator_list_t<expr::syntax_tree_t>& elements) -> std::optional<expr::symbol_table_t>;
        std::unordered_map<std::string, tta_t::graph_edge_iterator_t> edge_cache;
    };
}

#endif //AALTITOAD_INTERESTING_TOCKER_H
