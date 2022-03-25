#ifndef AALTITOAD_PIPE_TOCKER_H
#define AALTITOAD_PIPE_TOCKER_H
#include "runtime/tocker.h"
#include "runtime/ntta.h"

class pipe_tocker_t : public tocker_t {
    FILE* input_pipe;
    FILE* output_pipe;
    const ntta_t& associated_tta;
public:
    pipe_tocker_t() = delete;
    pipe_tocker_t(FILE* pipe, const ntta_t& tta);
    pipe_tocker_t(const std::string& pipe_filename, const ntta_t& tta);
    pipe_tocker_t(const std::string& input_pipe_filename, const std::string& output_pipe_filename, const ntta_t& tta);
    pipe_tocker_t(FILE* input_pipe, FILE* output_pipe, const ntta_t& tta);
    [[nodiscard]] symbol_table_t tock(const symbol_table_t& environment) const override;
};

extern "C" {
    pipe_tocker_t* create_pipe_tocker(const std::string& argument, const ntta_t& ntta);
    void destroy_pipe_tocker(pipe_tocker_t* tocker);
};

#endif //AALTITOAD_PIPE_TOCKER_H
