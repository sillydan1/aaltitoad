#ifndef CTL_INTERPRETER_H
#define CTL_INTERPRETER_H
#include <ctl-lang/ast-factory.h>
#include <ctl-lang/language-builder.h>
#include <ctl-scanner.hpp>
#include <ctl-parser.hpp>

namespace aaltitoad {
    class ctl_interpreter {
    public:
        ctl_interpreter(const expr::symbol_table_t& env1, const expr::symbol_table_t& env2);
        ctl_interpreter(const expr::symbol_table_t& env1); 
        ctl_interpreter(const std::initializer_list<std::reference_wrapper<expr::symbol_table_t>>& environments) : environments{environments} {}
        auto compile(const std::string& expression) -> ctl::syntax_tree_t {
            std::istringstream iss{expression};
            ctl::ast_factory factory{};
            ctl::multi_query_builder builder{};
            ctl::scanner scn{iss, std::cerr, &factory};
            ctl::parser_args pa{&scn, &factory, &builder};
            ctl::parser p{pa};
            if(p.parse() != 0)
                throw std::logic_error("unable to parse ctl query");
            auto res = builder.build();
            if(res.queries.size() != 1)
                throw std::logic_error("only one query must be provided");
            return res.queries[0];
        }
    private:
        std::vector<std::reference_wrapper<expr::symbol_table_t>> environments;
    };
}

#endif

