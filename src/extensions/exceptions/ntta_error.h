#ifndef AALTITOAD_NTTA_ERROR_H
#define AALTITOAD_NTTA_ERROR_H
#include <aaltitoadpch.h>
#include <nlohmann/json.hpp>
#include <spdlog/sinks/ostream_sink.h>

class ntta_error : public std::logic_error {
public:
    explicit ntta_error(const std::string& full_err_msg) : std::logic_error(full_err_msg.c_str()) {}
    static constexpr auto edge_update_err_format = "edge: <%s -> %s> - update: '%s' - error: %s";
    static constexpr auto update_err_format = "component: %s - edge: <%s -> %s> - update: '%s' - error: %s";

    static constexpr auto edge_guard_err_format = "edge: <%s -> %s> - guard: '%s' - error: %s";
    static constexpr auto guard_err_format = "component: %s - edge: <%s -> %s> - guard: '%s' - error: %s";
    static auto update_err_format_spdlog() {
        return string_format(update_err_format, "{0}", "{1}", "{2}", "{3}", "{4}");
    }
    static auto guard_err_format_spdlog() {
        return string_format(guard_err_format, "{0}", "{1}", "{2}", "{3}", "{4}");
    }
    static ntta_error edge_guard_error(const edge_t& edge, const expr::symbol_value_t& error_val) {
        return ntta_error(string_format(edge_guard_err_format,
                                     edge.from.c_str(), edge.to.c_str(),
                                     edge.guardExpression.c_str(),
                                     std::get<std::string>(error_val).c_str()));
    }
    static ntta_error edge_update_error(const edge_t& edge, const expr::symbol_value_t& error_val) {
        return ntta_error(string_format(edge_update_err_format,
                                     edge.from.c_str(), edge.to.c_str(),
                                     edge.guardExpression.c_str(),
                                     std::get<std::string>(error_val).c_str()));
    }
    static ntta_error in_component(const std::string& component_name, const ntta_error& other) {
        std::stringstream ss{};
        ss << "component: " << component_name;
        ss << " - " << other.what();
        return ntta_error(ss.str());
    }
};

#endif //AALTITOAD_NTTA_ERROR_H
