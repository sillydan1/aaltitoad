#ifndef AALTITOAD_PIPE_TOCKER_H
#define AALTITOAD_PIPE_TOCKER_H
#include "../../tocker.h"
#include "../../ntta.h"
#include "plugin_system/plugin_system.h"

class pipe_tocker_t : public tocker_t {
    mutable std::ifstream input_pipe;
    mutable std::ofstream output_pipe;
    const ntta_t& associated_tta;
public:
    pipe_tocker_t() = delete;
    pipe_tocker_t(const std::string& input_pipe_filename, const std::string& output_pipe_filename, const ntta_t& tta);
    [[nodiscard]] expr::symbol_table_t tock(const expr::symbol_table_t& environment) const override;
};

extern "C" {
    const char* get_plugin_name();
    const char* get_plugin_version();
    unsigned int get_plugin_type();
    pipe_tocker_t* create_tocker(const std::string& argument, const ntta_t& ntta);
};

#endif //AALTITOAD_PIPE_TOCKER_H
