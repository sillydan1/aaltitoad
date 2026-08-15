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
        Diagnostic result{};
        result.set_severity(diag.severity);
        result.set_modelkey(model_key);
        result.set_title(diag.title);
        result.set_lintidentifier(diag.identifier);
        result.set_message(diag.message);
        result.set_description(diag.description);
        for(auto& e : context)
            result.add_affectedelements(e);
        return result;
    }
}
