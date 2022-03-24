#ifndef AALTITOAD_ITOCKER_H
#define AALTITOAD_ITOCKER_H
#include <aaltitoadpch.h>
#include <future>

struct tocker_t {
    [[nodiscard]] virtual symbol_table_t tock(const symbol_table_t& environment) const = 0;
    virtual ~tocker_t() = default;
};

class async_tocker_t : public tocker_t {
protected:
    mutable std::future<symbol_table_t> job{};
    virtual symbol_table_t get_tock_values(const symbol_table_t& invocation_environment) const = 0;
    virtual void tock_async(const symbol_table_t& environment) const;
    ~async_tocker_t() override = default;

public:
    symbol_table_t tock(const symbol_table_t& environment) const override;
};

#endif //AALTITOAD_ITOCKER_H
