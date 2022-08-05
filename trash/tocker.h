#ifndef AALTITOAD_ITOCKER_H
#define AALTITOAD_ITOCKER_H
#include <aaltitoadpch.h>
#include <symbol_table.h>
#include <future>

struct tocker_t {
    [[nodiscard]] virtual expr::symbol_table_t tock(const expr::symbol_table_t& environment) const = 0;
    virtual ~tocker_t() = default;
};

class async_tocker_t : public tocker_t {
protected:
    mutable std::future<expr::symbol_table_t> job{};
    virtual expr::symbol_table_t get_tock_values(const expr::symbol_table_t& invocation_environment) const = 0;
    virtual void tock_async(const expr::symbol_table_t& environment) const {
        job = std::async([this, &environment](){ return get_tock_values(environment); });
    }
    ~async_tocker_t() override = default;

public:
    expr::symbol_table_t tock(const expr::symbol_table_t& environment) const override {
        if(job.valid()) {
            auto c = job.get();
            tock_async(environment);
            return c;
        }
        return {};
    }
};

#endif //AALTITOAD_ITOCKER_H
