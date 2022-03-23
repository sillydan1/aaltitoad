#ifndef AALTITOAD_ITOCKER_H
#define AALTITOAD_ITOCKER_H
#include <aaltitoadpch.h>
#include <future>

struct tocker_t {
    virtual symbol_table_t tock(const symbol_value_t& environment) = 0;
};

struct async_tocker_t : public tocker_t {
    std::future<symbol_table_t> job{};
    virtual symbol_table_t get_tock_values(const symbol_value_t& invocation_environment) = 0;
    virtual void tock_async(const symbol_value_t& environment);
    symbol_table_t tock(const symbol_value_t& environment) override;
};

#endif //AALTITOAD_ITOCKER_H
