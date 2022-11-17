#include <drivers/driver.h>
#include <nlohmann/json.hpp>
#include "query_json_loader.h"

namespace aaltitoad {
    auto load_query_json_file(const std::string& json_file, std::initializer_list<expr::symbol_table_ref_t> environments) -> std::vector<ctl::compiler::compiled_expr_t> {
        try {
            std::vector<ctl::compiler::compiled_expr_t> result{};
            ctl::compiler c{environments};
            std::ifstream f(json_file);
            auto data = nlohmann::json::parse(f);
            for(auto& q : data) {
                spdlog::trace("compiling query {0}", q["query"]);
                result.emplace_back(c.compile(q["query"]));
            }
            return result;
        } catch (std::exception &e) {
            spdlog::error("unable to parse json file {0}: {1}", json_file, e.what());
            throw e;
        }
    }
}
