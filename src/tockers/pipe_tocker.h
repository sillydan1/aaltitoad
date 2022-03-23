#ifndef AALTITOAD_PIPE_TOCKER_H
#define AALTITOAD_PIPE_TOCKER_H
#include "tockers.h"

class pipe_tocker_t : public tocker_t {
    FILE* input_pipe;
    FILE* output_pipe;
public:
    pipe_tocker_t() = delete;
    explicit pipe_tocker_t(FILE* pipe);
    pipe_tocker_t(FILE* input_pipe, FILE* output_pipe);
    symbol_table_t tock(const symbol_value_t& environment) override;
};

#endif //AALTITOAD_PIPE_TOCKER_H
