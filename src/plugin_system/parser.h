#ifndef AALTITOAD_PARSER_H
#define AALTITOAD_PARSER_H
#include "lsp.pb.h"
#include "ntta/tta.h"
#include <expected>

namespace aaltitoad::plugin {
    struct parse_error {
        std::vector<Diagnostic> diagnostics;
    };

    struct parse_ok {
        ntta_t ntta;
        std::vector<Diagnostic> diagnostics;
    };

    using parse_result = std::expected<parse_ok, parse_error>;

    class parser {
    public:
        virtual ~parser() = default;
        virtual auto parse_files(const std::vector<std::string>& files, const std::vector<std::string>& ignore_patterns) -> parse_result = 0 ;
        virtual auto parse_model(const Buffer& buffer) -> parse_result = 0;
    };
}

#endif
