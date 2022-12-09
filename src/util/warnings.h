#ifndef AALTITOAD_WARNINGS_H
#define AALTITOAD_WARNINGS_H
#include <unordered_map>
#include <string>
#include <vector>

namespace aaltitoad {
    enum w_t {
        overlap_idem,
        plugin_load_failed
    };

    class warnings {
    public:
        static auto is_enabled(const w_t& warning_name) -> bool;
        static void disable_warning(const w_t& warning_name);
        static void enable_all();
        static void disable_all();
        static auto descriptions() -> std::unordered_map<w_t, std::string>;
        static void warn(const w_t& warning, const std::string& msg);
        static void warn(const w_t& warning, const std::string& msg, const std::vector<std::string>& extra_info_lines);
    };
}

#endif //AALTITOAD_WARNINGS_H
