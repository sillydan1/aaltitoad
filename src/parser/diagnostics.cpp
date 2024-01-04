#include "diagnostics.h"

namespace aaltitoad {
    auto diagnostic_factory::with_model_key(const std::string& key) -> diagnostic_factory& {
        model_key = key;
        return *this;
    }

    auto diagnostic_factory::without_context() -> diagnostic_factory& {
        context.clear();
        return *this;
    }

    auto diagnostic_factory::with_context(const std::string& element) -> diagnostic_factory& {
        return with_context({element});
    }

    auto diagnostic_factory::with_context(const std::initializer_list<std::string>& elements) -> diagnostic_factory& {
        context.clear();
        for(auto& e : elements)
            context.push_back(e);
        return *this;
    }

    auto diagnostic_factory::with_context(const std::vector<std::string>& elements) -> diagnostic_factory& {
        context.clear();
        for(auto& e : elements)
            context.push_back(e);
        return *this;
    }

    auto diagnostic_factory::create_diagnostic(const diagnostic& diag) -> Diagnostic {
        return Diagnostic{
            .severity=diag.severity,
            .model_key=model_key,
            .title=diag.title,
            .lint_identifier=diag.identifier,
            .message=diag.message,
            .description=diag.description,
            .affected_elements=context
        };
    }
}
