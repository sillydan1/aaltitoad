#ifndef AALTITOAD_CTL_H
#define AALTITOAD_CTL_H
#include <string>
#include <drivers/compiler.h>

namespace aaltitoad {
    namespace ctl {
        enum path_quantifier {
            Forall, Exists
        };

        enum quantifier {
            Next, Globally, Finally,
            Until, WeakUntil
        };

        template<path_quantifier A, quantifier B, typename E = expr::compiler::compiled_expr_t>
        struct query_t {
            std::string query_string;
            E expression;
        };

        using ax_query = query_t<Forall, Next>;
        using ag_query = query_t<Forall, Globally>;
        using af_query = query_t<Forall, Finally>;
        using au_query = query_t<Forall, Until, std::pair<expr::compiler::compiled_expr_t, expr::compiler::compiled_expr_t>>;
        using aw_query = query_t<Forall, WeakUntil, std::pair<expr::compiler::compiled_expr_t, expr::compiler::compiled_expr_t>>;
        using ex_query = query_t<Forall, Next>;
        using eg_query = query_t<Forall, Globally>;
        using ef_query = query_t<Forall, Finally>;
        using eu_query = query_t<Forall, Until, std::pair<expr::compiler::compiled_expr_t, expr::compiler::compiled_expr_t>>;
        using ew_query = query_t<Forall, WeakUntil, std::pair<expr::compiler::compiled_expr_t, expr::compiler::compiled_expr_t>>;

        using query = std::variant<ax_query, ag_query, af_query, au_query, aw_query,
                                   ex_query, eg_query, ef_query, eu_query, ew_query>;
    }
}

#endif //AALTITOAD_CTL_H
