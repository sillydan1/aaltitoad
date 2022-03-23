#include "pipe_tocker.h"

pipe_tocker_t::pipe_tocker_t(FILE* pipe) : input_pipe{pipe}, output_pipe{pipe} {

}

pipe_tocker_t::pipe_tocker_t(FILE* input_pipe, FILE* output_pipe) : input_pipe{input_pipe}, output_pipe{output_pipe} {

}

symbol_table_t pipe_tocker_t::tock(const symbol_value_t &environment) {
    return {};
}
