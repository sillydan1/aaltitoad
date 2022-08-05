#ifndef AALTITOAD_ASYNC_TOCKER_H
#define AALTITOAD_ASYNC_TOCKER_H
#include "tta.h"

namespace aaltitoad {
    template<typename T>
    bool is_future_ready(std::future<T>& t){
        return t.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
    }

    class async_tocker_t : public tocker_t {
    protected:
        std::future<expr::symbol_table_t> job{};
        virtual void tock_async(const expr::symbol_table_t& environment) = 0;
        ~async_tocker_t() override = default;

    public:
        auto tock(const ntta_t& state) -> std::vector<expr::symbol_table_t> override {
            if(!job.valid())
                tock_async(state.symbols);
            if(!is_future_ready(job))
                return {};
            auto c = job.get();
            job = {};
            return {c};
        }
    };
}

#endif //AALTITOAD_ASYNC_TOCKER_H
