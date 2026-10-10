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
#include "network.h"
#include "problems.h"
#include <parser/hawk/scoped_template_builder/scoped_template_builder.h>
#include <util/exceptions/parse_error.h>
#include <iostream>
#include <regex>
#include <sstream>

namespace aaltitoad::graphedit {
    namespace {
        // Redirects std::cerr while it lives (the parser of expressions reports syntax errors there)
        class cerr_capture {
        public:
            cerr_capture() : previous{std::cerr.rdbuf(captured.rdbuf())} {}
            cerr_capture(const cerr_capture&) = delete;
            auto operator=(const cerr_capture&) -> cerr_capture& = delete;
            ~cerr_capture() { std::cerr.rdbuf(previous); }
            auto text() const -> std::string { return captured.str(); }
        private:
            std::stringstream captured{};
            std::streambuf* previous;
        };

        auto trimmed(const std::string& text) -> std::string {
            auto begin = text.find_first_not_of(" \t\r\n");
            if(begin == std::string::npos)
                return {};
            auto end = text.find_last_not_of(" \t\r\n");
            return text.substr(begin, end - begin + 1);
        }

        auto join(const std::vector<std::string>& parts, const std::string& separator) -> std::string {
            std::string result{};
            for(std::size_t i = 0; i < parts.size(); i++)
                result += (i == 0 ? "" : separator) + parts[i];
            return result;
        }

        // The parameters of a template ("Light(id, n)") or the arguments of an invocation ("lamp(1, 2)"), split as
        // the scoped template builder of aaltitoad splits them
        auto split_signature(const std::string& text) -> std::vector<std::string> {
            static const std::regex section{R"(\(.+(,.+)*\))"};
            static const std::regex split{R"([\""].*[\""]|[^,]+)"};
            std::vector<std::string> result{};
            std::smatch match{};
            if(!std::regex_search(text.cbegin(), text.cend(), match, section))
                return result;
            auto inner = match.str().substr(1, match.str().size() - 2);
            for(auto it = std::sregex_iterator(inner.begin(), inner.end(), split); it != std::sregex_iterator(); ++it)
                result.push_back(trimmed(it->str()));
            return result;
        }

        void add_problem(network_t& network, const std::string& key, problem_t problem) {
            auto& problems = network.problems[key];
            if(std::find(problems.begin(), problems.end(), problem) == problems.end())
                problems.push_back(std::move(problem));
        }

        auto invalid_expression(const std::string& title, const std::string& error, std::vector<std::string> elements) -> problem_t {
            return {problems::invalid_expression, severity_t::error, title, error, std::move(elements)};
        }

        // Report templates with the same name and a missing (or a second) main template. Returns whether there are
        // any.
        auto check_names(network_t& network, const std::vector<const ntta_model_t*>& templates) -> bool {
            std::map<std::string, std::vector<const ntta_model_t*>> by_name{};
            std::vector<const ntta_model_t*> mains{};
            for(auto* model : templates) {
                by_name[model->name].push_back(model);
                if(model->is_main)
                    mains.push_back(model);
            }
            auto found = false;
            for(auto& [name, models] : by_name) {
                if(models.size() < 2)
                    continue;
                found = true;
                for(auto* model : models)
                    add_problem(network, model->key, {problems::duplicate_names, severity_t::error,
                                                      "Several templates have the same name",
                                                      std::to_string(models.size()) + " templates are named '" + name +
                                                      "': aaltitoad cannot tell them apart", {}});
            }
            if(mains.empty() && !templates.empty()) {
                found = true;
                for(auto* model : templates)
                    add_problem(network, model->key, {problems::no_main_template, severity_t::error,
                                                      "The network has no main template",
                                                      "aaltitoad starts a network with its main template: make one "
                                                      "template the main template (the metadata is_main)", {}});
            }
            if(mains.size() > 1) {
                found = true;
                for(auto* model : mains)
                    add_problem(network, model->key, {problems::several_main_templates, severity_t::error,
                                                      "The network has several main templates",
                                                      std::to_string(mains.size()) + " templates are the main "
                                                      "template, but a network starts with one", {}});
            }
            return found;
        }

        void check_locations(network_t& network, const ntta_model_t& model) {
            std::vector<std::string> initial{};
            std::vector<std::string> final{};
            for(auto& location : model.locations) {
                if(location.kind == location_kind_t::initial)
                    initial.push_back(location.id);
                if(location.kind == location_kind_t::final)
                    final.push_back(location.id);
            }
            if(initial.size() == 1 && final.size() == 1)
                return;
            auto count = [](std::size_t n, const std::string& what) {
                if(n < 2)
                    return (n == 0 ? "no " : "one ") + what + " location";
                return std::to_string(n) + " " + what + " locations";
            };
            std::vector<std::string> elements{};
            if(initial.size() > 1)
                elements.insert(elements.end(), initial.begin(), initial.end());
            if(final.size() > 1)
                elements.insert(elements.end(), final.begin(), final.end());
            add_problem(network, model.key, {problems::initial_and_final_locations, severity_t::error,
                                             "The template needs one initial and one final location",
                                             "aaltitoad runs templates with exactly one initial and one final location, "
                                             "but this one has " + count(initial.size(), "initial") + " and " +
                                             count(final.size(), "final"), elements});
        }

        void check_expressions(network_t& network, const ntta_model_t& model) {
            auto [declarations, environment] = strip_environment_modifiers(model.declarations);
            if(environment)
                add_problem(network, model.key, {problems::unsupported_declaration, severity_t::error,
                                                 "aaltitoad does not know extern and persistent declarations",
                                                 "Templates of aaltitoad declare internal variables only: declare the "
                                                 "variables of the environment in a model without vertices", {}});
            else if(auto parsed = parse_expression(declarations); !parsed.result)
                add_problem(network, model.key, invalid_expression("aaltitoad cannot parse the declarations", parsed.error, {}));
            else if(parsed.result->expression)
                add_problem(network, model.key, invalid_expression("The declarations are not declarations",
                                                                   "The declarations are an expression, not a sequence "
                                                                   "of declarations", {}));
            for(auto& edge : model.edges) {
                if(!edge.guard.empty()) {
                    auto parsed = parse_expression(edge.guard);
                    if(!parsed.result)
                        add_problem(network, model.key, invalid_expression("aaltitoad cannot parse the guard", parsed.error, edge.guard_nails));
                    else if(!parsed.result->expression || !parsed.result->declarations.empty())
                        add_problem(network, model.key, invalid_expression("The guard is not an expression",
                                                                           "A guard is a boolean expression, e.g. x > 2 && !done",
                                                                           edge.guard_nails));
                }
                if(!edge.update.empty()) {
                    auto parsed = parse_expression(edge.update);
                    if(!parsed.result)
                        add_problem(network, model.key, invalid_expression("aaltitoad cannot parse the update", parsed.error, edge.update_nails));
                    else if(parsed.result->expression)
                        add_problem(network, model.key, invalid_expression("The update is not a sequence of assignments",
                                                                           "An update assigns variables, e.g. x := x + 1; done := true",
                                                                           edge.update_nails));
                }
            }
        }

        void check_subcomponents(network_t& network, const ntta_model_t& model,
                                 const std::map<std::string, const ntta_model_t*>& templates) {
            for(auto& subcomponent : model.subcomponents) {
                auto target = templates.find(subcomponent.template_name);
                if(subcomponent.template_name.empty() || target == templates.end()) {
                    add_problem(network, model.key, {problems::unknown_template, severity_t::error,
                                                     "The subcomponent instantiates no template",
                                                     subcomponent.template_name.empty()
                                                     ? "The subcomponent names no template"
                                                     : "There is no template named '" + subcomponent.template_name + "'",
                                                     {subcomponent.id}});
                    continue;
                }
                if(subcomponent.invocation.empty()) {
                    add_problem(network, model.key, {problems::argument_mismatch, severity_t::error,
                                                     "The subcomponent has no invocation",
                                                     "The invocation names the instance (e.g. lamp(1))", {subcomponent.id}});
                    continue;
                }
                auto parameters = split_signature(subcomponent.template_name);
                auto arguments = split_signature(subcomponent.invocation);
                if(parameters.size() != arguments.size()) {
                    add_problem(network, model.key, {problems::argument_mismatch, severity_t::error,
                                                     "The invocation does not match the template",
                                                     "'" + subcomponent.template_name + "' has " +
                                                     std::to_string(parameters.size()) + " parameters, but '" +
                                                     subcomponent.invocation + "' gives " +
                                                     std::to_string(arguments.size()) + " arguments", {subcomponent.id}});
                    continue;
                }
                for(auto& argument : arguments) {
                    auto parsed = parse_expression(argument);
                    if(!parsed.result || !parsed.result->expression)
                        add_problem(network, model.key, invalid_expression("aaltitoad cannot parse an argument",
                                                                           parsed.result ? "'" + argument + "' is not an expression" : parsed.error,
                                                                           {subcomponent.id}));
                }
            }
        }

        // Report the subcomponents through which templates instantiate each other in a circle. Returns whether there
        // are any.
        auto check_recursion(network_t& network, const std::map<std::string, const ntta_model_t*>& templates) -> bool {
            auto reaches = [&templates](const std::string& from, const std::string& to) {
                std::set<std::string> visited{};
                std::vector<std::string> stack{from};
                while(!stack.empty()) {
                    auto name = stack.back();
                    stack.pop_back();
                    if(name == to)
                        return true;
                    if(!visited.insert(name).second || !templates.contains(name))
                        continue;
                    for(auto& subcomponent : templates.at(name)->subcomponents)
                        stack.push_back(subcomponent.template_name);
                }
                return false;
            };
            auto found = false;
            for(auto& [name, model] : templates) {
                for(auto& subcomponent : model->subcomponents) {
                    if(!templates.contains(subcomponent.template_name) || !reaches(subcomponent.template_name, name))
                        continue;
                    found = true;
                    add_problem(network, model->key, {problems::recursive_instantiation, severity_t::error,
                                                      "Templates instantiate each other in a circle",
                                                      "'" + subcomponent.template_name + "' leads back to '" + name +
                                                      "', so the network would never end", {subcomponent.id}});
                }
            }
            return found;
        }

        // The templates that the network instantiates, starting with the main template
        auto reachable_templates(const ntta_model_t& main, const std::map<std::string, const ntta_model_t*>& templates) -> std::set<std::string> {
            std::set<std::string> result{};
            std::vector<std::string> stack{main.name};
            while(!stack.empty()) {
                auto name = stack.back();
                stack.pop_back();
                if(!templates.contains(name) || !result.insert(name).second)
                    continue;
                for(auto& subcomponent : templates.at(name)->subcomponents)
                    stack.push_back(subcomponent.template_name);
            }
            return result;
        }

        auto to_hawk(const ntta_model_t& model) -> hawk::model::tta_template {
            hawk::model::tta_template result{.name=model.name, .declarations=model.declarations, .is_main=model.is_main,
                                             .locations={}, .edges={}, .initial_location={}, .final_location={},
                                             .instances={}};
            for(auto& l : model.locations) {
                hawk::model::location_t location{.id=l.id, .nickname=l.nickname, .invariant="",
                                                 .urgency=l.immediate ? hawk::model::urgency_t::urgent : hawk::model::urgency_t::normal};
                if(l.kind == location_kind_t::initial)
                    result.initial_location = location;
                else if(l.kind == location_kind_t::final)
                    result.final_location = location;
                else
                    result.locations.push_back(location);
            }
            for(auto& e : model.edges)
                result.edges.push_back({.id=e.id, .source=e.source, .target=e.target, .guard=e.guard, .update=e.update});
            for(auto& s : model.subcomponents)
                result.instances.push_back({.id=s.id, .tta_template_name=s.template_name, .invocation=s.invocation});
            return result;
        }

        // Report the variables that guards and updates use, but no model declares
        void check_identifiers(network_t& network) {
            const auto& automata = *network.automata;
            auto declared = [&automata](const std::string& identifier) {
                return automata.symbols.contains(identifier) || automata.external_symbols.contains(identifier);
            };
            // (model, edge, identifier, in the guard) -> the instances in which it is not declared
            std::map<std::tuple<std::string, std::string, std::string, bool>, std::set<std::string>> undeclared{};
            for(auto& [name, tta] : automata.components) {
                for(auto& [data, edge] : tta.graph->edges) {
                    auto it = network.edges.find(data.identifier);
                    if(it == network.edges.end())
                        continue;
                    const auto& model = it->second.model;
                    const auto& id = it->second.edge.id;
                    for(auto& identifier : identifiers_of(data.guard))
                        if(!declared(identifier))
                            undeclared[{model, id, identifier, true}].insert(name);
                    for(auto& [assigned, value] : data.updates) {
                        if(!declared(assigned))
                            undeclared[{model, id, assigned, false}].insert(name);
                        for(auto& identifier : identifiers_of(value))
                            if(!declared(identifier))
                                undeclared[{model, id, identifier, false}].insert(name);
                    }
                }
            }
            for(auto& [key, instances] : undeclared) {
                const auto& [model, edge_id, identifier, in_guard] = key;
                const auto& edges = network.models.at(model).edges;
                auto edge = std::find_if(edges.begin(), edges.end(), [&edge_id](const auto& e){ return e.id == edge_id; });
                if(edge == edges.end())
                    continue;
                auto message = "'" + identifier + "' is not declared";
                if(instances.size() < network.instances_of(model).size())
                    message += " (in " + join({instances.begin(), instances.end()}, ", ") + ")";
                add_problem(network, model, {problems::undeclared_identifier, severity_t::error,
                                             in_guard ? "The guard uses a variable that is not declared"
                                                      : "The update uses a variable that is not declared",
                                             message, in_guard ? edge->guard_nails : edge->update_nails});
            }
        }
    }

    auto network_t::has_errors() const -> bool {
        for(auto& [key, list] : problems)
            for(auto& problem : list)
                if(problem.severity == severity_t::error)
                    return true;
        return false;
    }

    auto network_t::instances_of(const std::string& model) const -> std::vector<const instance_t*> {
        std::vector<const instance_t*> result{};
        for(auto& [name, instance] : instances)
            if(instance.model == model)
                result.push_back(&instance);
        return result;
    }

    auto parse_expression(const std::string& text) -> parse_result_t {
        return parse_with([&text](){ return expression_driver{}.parse(text); });
    }

    auto parse_with(const std::function<expression_driver::language_result()>& parse) -> parse_result_t {
        cerr_capture capture{};
        try {
            return {parse(), {}};
        } catch(const std::exception& e) {
            auto reported = capture.text();
            auto line = trimmed(reported.substr(0, reported.find('\n')));
            if(!line.empty() && line.back() == ':')
                line.pop_back();
            return {std::nullopt, line.empty() ? std::string{e.what()} : line};
        }
    }

    auto strip_environment_modifiers(const std::string& declarations) -> std::pair<std::string, bool> {
        std::string result{};
        auto found = false;
        auto is_word = [](char c){ return std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '.' || c == '(' || c == ')'; };
        for(std::size_t i = 0; i < declarations.size();) {
            auto c = declarations[i];
            if(c == '"') {
                // a string literal, with escapes
                auto end = i + 1;
                while(end < declarations.size() && declarations[end] != '"')
                    end += declarations[end] == '\\' ? 2 : 1;
                end = std::min(end + 1, declarations.size());
                result += declarations.substr(i, end - i);
                i = end;
                continue;
            }
            if(!is_word(c)) {
                result += c;
                i++;
                continue;
            }
            auto end = i;
            while(end < declarations.size() && is_word(declarations[end]))
                end++;
            auto word = declarations.substr(i, end - i);
            if(word == "extern" || word == "persistent") {
                found = true;
                while(end < declarations.size() && std::isspace(static_cast<unsigned char>(declarations[end])))
                    end++;
            } else {
                result += word;
            }
            i = end;
        }
        return {result, found};
    }

    auto identifiers_of(const expr::syntax_tree_t& expression) -> std::set<std::string> {
        std::set<std::string> result{};
        std::vector<const expr::syntax_tree_t*> stack{&expression};
        while(!stack.empty()) {
            const auto* tree = stack.back();
            stack.pop_back();
            if(const auto* identifier = std::get_if<expr::identifier_t>(&static_cast<const expr::underlying_syntax_node_t&>(tree->node)))
                result.insert(identifier->ident);
            for(auto& child : tree->children())
                stack.push_back(&child);
        }
        return result;
    }

    auto build_network(std::map<std::string, ntta_model_t> models) -> network_t {
        network_t network{};
        network.models = std::move(models);
        std::vector<const ntta_model_t*> templates{};
        std::map<std::string, const ntta_model_t*> templates_by_name{};
        for(auto& [key, model] : network.models) {
            network.problems[key] = model.problems;
            if(model.is_template()) {
                templates.push_back(&model);
                templates_by_name.emplace(model.name, &model);
            }
        }

        // what prevents aaltitoad from building the network
        auto blocked = check_names(network, templates);
        blocked = check_recursion(network, templates_by_name) || blocked;
        for(auto* model : templates) {
            check_locations(network, *model);
            check_expressions(network, *model);
            check_subcomponents(network, *model, templates_by_name);
        }
        std::vector<std::string> environment{};
        for(auto& [key, model] : network.models) {
            if(model.is_template())
                continue;
            auto declarations = strip_environment_modifiers(model.declarations).first;
            auto parsed = parse_expression(declarations);
            if(!parsed.result)
                add_problem(network, key, invalid_expression("aaltitoad cannot parse the declarations", parsed.error, {}));
            else if(parsed.result->expression)
                add_problem(network, key, invalid_expression("The declarations are not declarations",
                                                             "The declarations are an expression, not a sequence of "
                                                             "declarations", {}));
            else if(!trimmed(declarations).empty())
                environment.push_back(declarations);
        }
        // problems of templates that the network does not instantiate do not prevent it from being built
        const ntta_model_t* main = nullptr;
        for(auto* model : templates)
            if(model->is_main)
                main = model;
        if(blocked || main == nullptr)
            return network;
        auto reachable = reachable_templates(*main, templates_by_name);
        for(auto& [key, problems] : network.problems) {
            auto& model = network.models.at(key);
            if(model.is_template() && !reachable.contains(model.name))
                continue;
            for(auto& problem : problems)
                if(problem.severity == severity_t::error)
                    return network;
        }

        hawk::scoped_template_builder builder{};
        for(auto* model : templates)
            builder.add_template(to_hawk(*model));
        for(auto& declarations : environment)
            builder.add_global_symbols(declarations);
        try {
            cerr_capture capture{};
            network.automata.reset(builder.build_heap());
        } catch(const aggregate_error& e) {
            add_problem(network, main->key, {problems::build_failed, severity_t::error,
                                             "aaltitoad cannot build the network", e.message(), {}});
            return network;
        } catch(const std::exception& e) {
            add_problem(network, main->key, {problems::build_failed, severity_t::error,
                                             "aaltitoad cannot build the network", e.what(), {}});
            return network;
        }
        for(auto& built : builder.get_instances()) {
            auto model = templates_by_name.at(built.instance.tta_template_name)->key;
            network.instances[built.name] = {built.name, model, built.parent, built.parent.empty() ? "" : built.instance.id,
                                             built.local_identifiers, built.parameters};
            const auto& edges = network.models.at(model).edges;
            for(auto& [edge_id, identifier] : built.edge_identifiers) {
                auto edge = std::find_if(edges.begin(), edges.end(), [&edge_id](const auto& e){ return e.id == edge_id; });
                if(edge != edges.end())
                    network.edges[identifier] = {built.name, model, *edge};
            }
        }
        for(auto& [name, value] : network.automata->external_symbols)
            network.external_symbols.insert(name);
        check_identifiers(network);
        return network;
    }
}
