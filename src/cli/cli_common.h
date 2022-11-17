#ifndef AALTITOAD_CLI_COMMON_H
#define AALTITOAD_CLI_COMMON_H
#include <vector>
#include <argvparse.h>
#include <iostream>
#include <config.h>
#include <magic_enum.hpp>
#include <util/warnings.h>

int print_required_args() {
    std::cout << "Required arguments:\n";
    std::cout << " --input\n";
    return 1;
}

int print_help(const std::string& program_name, const std::vector<option_t>& options) {
    std::cout << get_license() << std::endl;
    std::cout << PROJECT_NAME << " v" << PROJECT_VER << std::endl;
    std::cout << "USAGE: " << program_name << " -i /path/to/tta/dir [OPTIONS] \n" << std::endl;
    std::cout << "OPTIONS: " << std::endl;
    print_argument_help(options);
    return 0;
}

int print_version() {
    std::cout << PROJECT_NAME << " v" << PROJECT_VER << std::endl;
    return 0;
}

void disable_warnings(const std::vector<std::string>& disable_warns) {
    aaltitoad::warnings::enable_all();
    for(auto& w : disable_warns) {
        auto opt = magic_enum::enum_cast<aaltitoad::w_t>(w);
        if(opt.has_value())
            aaltitoad::warnings::disable_warning(opt.value());
        else
            spdlog::warn("not a warning: {0}", w);
    }
}

int list_warnings() {
    std::cout << "[WARNINGS]:\n";
    for(const auto& warning : aaltitoad::warnings::descriptions())
        std::cout << "\t - [" << magic_enum::enum_name(warning.first) << "]: " << warning.second << "\n";
    return 0;
}

#endif //AALTITOAD_CLI_COMMON_H
