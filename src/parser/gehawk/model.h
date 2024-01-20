#ifndef GEHAWK_MODEL_H
#define GEHAWK_MODEL_H

namespace aaltitoad::gehawk::model {
    enum class location_type_t {
        normal=0,
        initial,
        final,
        invalid=-1
    };

    enum class immediacy_t {
        normal=0,
        immediate,
        invalid=-1
    };

    struct location_t {
        location_type_t type;
        std::string nickname;
        immediacy_t immediacy;
    };

    struct tta_template_t {

    };
}

#endif // !GEHAWK_MODEL_H
