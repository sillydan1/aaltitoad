#include <aaltitoadpch.h>
#include <ntta/tta.h>
#include <timer>
#include <plugin_system/plugin_system.h>
#include <ctl_compiler.h>
#include <verification/forward_reachability.h>
#include <ntta/interesting_tocker.h>
#include "cli_options.h"
#include "../cli_common.h"
#include "query/query_json_loader.h"

auto load_plugins(std::map<std::string, argument_t>& cli_arguments) -> plugin_map_t;

int main(int argc, char** argv) {
    try {
        auto options = get_options();
        auto cli_arguments = get_arguments(options, argc, argv);
        if(cli_arguments["verbosity"])
            spdlog::set_level(static_cast<spdlog::level::level_enum>(SPDLOG_LEVEL_OFF - cli_arguments["verbosity"].as_integer()));
        if(cli_arguments["version"])
            return print_version();
        if(cli_arguments["list-warn"])
            return list_warnings();
        if(cli_arguments["help"])
            return print_help(argv[0], options);
        if(!is_required_provided(cli_arguments, options))
            return print_required_args();
        spdlog::trace("welcome to {0} v{1}", PROJECT_NAME, PROJECT_VER);
        if(cli_arguments["no-warn"])
            aaltitoad::warnings::disable_all();
        disable_warnings(cli_arguments["disable-warn"].as_list_or_default({}));

        auto available_plugins = load_plugins(cli_arguments);
        if(cli_arguments["list-plugins"]) {
            std::cout << "available plugins:\n" << available_plugins;
            return 0;
        }

        auto selected_parser = cli_arguments["parser"].as_string_or_default("huppaal_parser");
        if(!available_plugins.contains(selected_parser) || available_plugins.at(selected_parser).type != plugin_type::parser) {
            spdlog::critical("no such parser available: '{0}'", selected_parser);
            return 1;
        }

        spdlog::debug("parsing with {0} plugin", selected_parser);
        auto inputs = cli_arguments["input"].as_list();
        auto ignore = cli_arguments["ignore"].as_list_or_default({});
        auto parser = std::get<parser_func_t>(available_plugins.at(selected_parser).function);
        ya::timer<int> t{};
        std::unique_ptr<aaltitoad::ntta_t> n{parser(inputs, ignore)};
        spdlog::debug("model parsing took {0}ms", t.milliseconds_elapsed());

        t.start();
        std::vector<ctl::compiler::compiled_expr_t> queries{};
        ctl::compiler ctl_compiler{{n->symbols, n->external_symbols}};
        for(auto& q : cli_arguments["query"].as_list_or_default({})) {
            spdlog::trace("compiling query '{0}'", q);
            queries.emplace_back(ctl_compiler.compile(q));
        }
        for(auto& f : cli_arguments["query-file"].as_list_or_default({})) {
            spdlog::trace("loading queries in file {0}", f);
            auto json_queries = aaltitoad::load_query_json_file(f, {n->symbols, n->external_symbols});
            queries.insert(queries.end(), json_queries.begin(), json_queries.end());
        }
        // TODO: filter unsupported queries out
        spdlog::debug("query parsing took {0}ms", t.milliseconds_elapsed());

        n->add_tocker(std::make_unique<aaltitoad::interesting_tocker>());
        spdlog::trace("starting reachability search for {0} queries", queries.size());
        t.start();
        aaltitoad::forward_reachability_searcher frs{};
        auto results = frs.is_reachable(*n, queries);
        spdlog::debug("reachability search took {0}ms", t.milliseconds_elapsed());

        // gather and return results
        for(auto& result : results) {
            std::cout << result.query << ": " << std::boolalpha << result.solution.has_value() << "\n";
            if(result.solution.has_value())
                std::cout << result.solution.value();
        }

        return 0;
    } catch (std::exception& any) {
        spdlog::error(any.what());
        std::cout.flush();
        std::cerr.flush();
        return 1;
    }
}

auto load_plugins(std::map<std::string, argument_t>& cli_arguments) -> plugin_map_t {
    auto rpath = std::getenv("AALTITOAD_LIBPATH");
    std::vector<std::string> look_dirs = { ".", "lib", "plugins" };
    if(rpath)
        look_dirs.emplace_back(rpath);
    auto provided_dirs = cli_arguments["plugin-dir"].as_list_or_default({});
    look_dirs.insert(look_dirs.end(), provided_dirs.begin(), provided_dirs.end());
    return aaltitoad::plugins::load(look_dirs);
}
