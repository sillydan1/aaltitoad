#include <aaltitoadpch.h>
#include <config.h>
#include "cli_options.h"

int main(int argc, char** argv) {
    auto options = get_options();
    auto cli_arguments = get_arguments(options, argc, argv);

    return 0;
}
