#include "warnings.h"

namespace aaltitoad {
    auto warnings::is_enabled(const std::string &warning_name) -> bool {
        return !disabled_warnings[warning_name];
    }

    void warnings::disable_warning(const std::string &warning_name) {
        disabled_warnings[warning_name] = true;
    }
}
