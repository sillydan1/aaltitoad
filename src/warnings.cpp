#include "warnings.h"
#include <magic_enum.hpp>

namespace aaltitoad {
    static bool default_value = false;
    static std::unordered_map<w_t, bool> enabled_warnings = {};

    auto warnings::is_enabled(const w_t &warning_name) -> bool {
        if (enabled_warnings.contains(warning_name))
            return enabled_warnings[warning_name];
        return default_value;
    }

    void warnings::disable_warning(const w_t &warning_name) {
        spdlog::debug("disabling warning [{0}]", magic_enum::enum_name(warning_name));
        enabled_warnings[warning_name] = false;
    }

    void warnings::enable_all() {
        default_value = true;
    }

    auto warnings::descriptions() -> std::unordered_map<w_t, std::string> {
        return {
            {w_t::overlap_idem, "warnings about overlapping non-idempotent symbol table changes"},
            {w_t::plugin_load_failed, "warnings about plugins failing to load"},
        };
    }

    void warnings::warn(const aaltitoad::w_t &warning, const std::string &msg) {
        if(is_enabled(warning))
            spdlog::warn("[{0}]: {1}", magic_enum::enum_name(warning), msg);
    }
}
