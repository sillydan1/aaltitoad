#include "tockers.h"

void async_tocker_t::tock_async(const symbol_table_t& environment) const {
    job = std::async([this, &environment](){ return get_tock_values(environment); });
}

symbol_table_t async_tocker_t::tock(const symbol_table_t& environment) const {
    if(job.valid()) {
        auto c = job.get();
        tock_async(environment);
        return c;
    }
    return {};
}
