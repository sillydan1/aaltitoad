#ifndef AALTITOAD_HAWK_DIAGNOSTICS_H
#define AALTITOAD_HAWK_DIAGNOSTICS_H
#include "lsp.pb.h"

namespace aaltitoad {
    struct diagnostic {
        std::string identifier;
        std::string title;
        std::string message;
        std::string description;
        Severity severity;
    };

    class diagnostic_factory {
        std::string model_key;
        std::vector<std::string> context;
        // TODO: suggestion generator
    public:
        diagnostic_factory() = default;
        auto without_context() -> diagnostic_factory&;
        auto with_model_key(const std::string& key) -> diagnostic_factory&;
        auto with_context(const std::string& element) -> diagnostic_factory&;
        auto with_context(const std::initializer_list<std::string>& elements) -> diagnostic_factory&;
        auto with_context(const std::vector<std::string>& elements) -> diagnostic_factory&;
        auto create_diagnostic(const diagnostic& diag) -> Diagnostic;
    };
}

#endif // !AALTITOAD_HAWK_DIAGNOSTICS_H
