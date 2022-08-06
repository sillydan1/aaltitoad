#ifndef AALTITOAD_WARNINGS_H
#define AALTITOAD_WARNINGS_H
#include <unordered_map>
#include <string>

namespace aaltitoad {
    class warnings {
        static std::unordered_map<std::string, bool> disabled_warnings; // NOTE: This implies -Wall is always on
    public:
        static auto is_enabled(const std::string& warning_name) -> bool;
        static void disable_warning(const std::string& warning_name);
    };
}

#endif //AALTITOAD_WARNINGS_H
