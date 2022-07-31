#ifndef AALTITOAD_ASYNC_TOCKER_H
#define AALTITOAD_ASYNC_TOCKER_H
#include "tta.h"

namespace aaltitoad {
    class async_tocker_t : public tocker_t {
    protected:
        mutable std::future<expr::symbol_table_t> job{};
        virtual expr::symbol_table_t get_tock_values(const expr::symbol_table_t& invocation_environment) const = 0;
        virtual void tock_async(const expr::symbol_table_t& environment) const {
            job = std::async([this, &environment](){
                return get_tock_values(environment);
            });
        }
        ~async_tocker_t() override = default;

    public:
        auto tock(const ntta_t& state) -> std::vector<expr::symbol_table_t> override {
            if(!job.valid())
                return {};
            auto c = job.get();
            tock_async(state.symbols);
            return {c};
        }
    };
}

#endif //AALTITOAD_ASYNC_TOCKER_H
