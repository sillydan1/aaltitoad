#include <random>
#include "random.h"

namespace aaltitoad::random {
    std::random_device r;
    auto value(int min, int max) -> int {
        std::default_random_engine e1(r());
        return std::uniform_int_distribution<int>(min,max)(e1);
    }

    auto value(double min, double max) -> double {
        std::default_random_engine e1(r());
        return std::uniform_real_distribution<double>(min, max)(e1);
    }
}

