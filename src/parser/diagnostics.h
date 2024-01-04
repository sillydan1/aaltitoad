#ifndef AALTITOAD_HAWK_DIAGNOSTICS_H
#define AALTITOAD_HAWK_DIAGNOSTICS_H
#include <initializer_list>
#include <string>
#include <vector>

namespace aaltitoad {
    enum class Severity {
        SEVERITY_HINT,
        SEVERITY_INFO,
        SEVERITY_WARNING,
        SEVERITY_ERROR
    };

    struct Diagnostic {
        Severity severity;
        std::string model_key;
        std::string title;
        std::string lint_identifier;
        std::string message;
        std::string description;
        std::vector<std::string> affected_elements;
    };

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
