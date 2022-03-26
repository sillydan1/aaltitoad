#include "pipe_tocker.h"

pipe_tocker_t::pipe_tocker_t(FILE* pipe, const ntta_t& ntta)
 : associated_tta{ntta}, input_pipe{pipe}, output_pipe{pipe} {

}

pipe_tocker_t::pipe_tocker_t(FILE* input_pipe, FILE* output_pipe, const ntta_t& ntta)
 : associated_tta{ntta}, input_pipe{input_pipe}, output_pipe{output_pipe} {

}

void Pprint_state(const ntta_t& automata) {
    std::stringstream ss{};
    ss << stream_mods::json << automata;
    spdlog::info(ss.str());
}

symbol_table_t pipe_tocker_t::tock(const symbol_table_t &environment) const {
    Pprint_state(associated_tta);
    return {};
}

extern "C" {
    const char* get_plugin_name() {
        return "pipe_tocker";
    }

    pipe_tocker_t* create_pipe_tocker(const std::string &argument, const ntta_t& ntta) {
        auto f = fopen(argument.c_str(), "rw");
        return new pipe_tocker_t{f, ntta};
    }

    void destroy_pipe_tocker(pipe_tocker_t* tocker) {
        delete tocker;
    }
}
