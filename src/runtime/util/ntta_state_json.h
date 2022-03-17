#ifndef AALTITOAD_NTTA_STATE_JSON_H
#define AALTITOAD_NTTA_STATE_JSON_H

namespace stream_mods {
    class ntta_state_json {};
    constexpr ntta_state_json json;
}

struct json_ostream {
    std::ostream& os;
};

inline json_ostream operator<<(std::ostream& os, stream_mods::ntta_state_json) {
    return { os };
}

template <typename T>
std::ostream& operator<<(json_ostream jos, const T& v) {
    return jos.os << v;
}

#endif //AALTITOAD_NTTA_STATE_JSON_H
