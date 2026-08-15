/**
 * aaltitoad - a verification engine for tick tock automata models
   Copyright (C) 2023 Asger Gitz-Johansen

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include "compiler.h"
#include <overload>

namespace aaltitoad::hawk {
    auto not_implemented_yet() -> diagnostic {
        return diagnostic{
        .identifier="not_implemented_yet",
        .title="Function not implemented yet",
        .message="The called function is not implemented yet",
        .description="The function is not able to be called and is not implemented yet.",
        .severity=Severity::SEVERITY_ERROR};
    }

    compiler::compiler(
            scanner& _scanner,
            parser& _parser,
            semantic_analyzer& _analyzer,
            optimizer& _optimizer,
            generator& _generator)
        : 
            _scanner{_scanner},
            _parser{_parser},
            _analyzer{_analyzer},
            _optimizer{_optimizer},
            _generator{_generator},
            symbols{},
            _diagnostic_factory{} {
    }

    void compiler::add_symbols(const expr::symbol_table_t& symbols) {
        this->symbols.put(symbols);
    }

    void compiler::clear_symbols() {
        symbols.clear();
    }

    auto compiler::compile(const std::vector<std::string>& paths, const std::vector<std::string>& ignore_list) -> std::expected<ok, error_t> {
        auto stream = _scanner.scan(*this, paths, ignore_list);
        if(!stream)
            return std::unexpected{stream.error()};
        auto ast = _parser.parse(*this, stream.value());
        if(!ast)
            return std::unexpected{ast.error()};
        auto dast_r = _analyzer.analyze(*this, ast.value());
        if(!dast_r)
            return std::unexpected{dast_r.error()};
        auto dast = dast_r.value();
        _optimizer.optimize(*this, dast);
        auto ntta = _generator.generate(*this, dast);
        if(!ntta)
            return std::unexpected(ntta.error());
        return ok{
            ntta.value(),
            dast.diagnostics
        };
    }

    auto compiler::get_diagnostic_factory() -> diagnostic_factory& {
        return _diagnostic_factory;
    }

    auto compiler::diag(const diagnostic& d) -> Diagnostic {
        return get_diagnostic_factory().without_context().create_diagnostic(d);
    }

    error_t::error_t() : diagnostics{} {}

    error_t::error_t(const std::vector<Diagnostic>& diagnostics) : diagnostics{diagnostics} {}

    void nothing_optimizer::optimize(compiler& ctx, parser::ok& ast) const {
        
    }
}
