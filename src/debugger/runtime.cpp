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
#include "runtime.h"
#include <parser/hawk/scoped_template_builder/scoped_interpreter.h>
#include <charconv>
#include <climits>
#include <utility>
#include <variant>

namespace aaltitoad::debugger {
    namespace {
        constexpr auto stuck_description = "No edge is enabled: change the variables of the environment to continue";

        auto base(const expr::symbol_value_t& value) -> const expr::underlying_symbol_value_t& {
            return value;
        }

        auto type_of(const expr::symbol_value_t& value) -> std::string {
            const auto& v = base(value);
            if(std::holds_alternative<int>(v))
                return "int";
            if(std::holds_alternative<float>(v))
                return "float";
            if(std::holds_alternative<bool>(v))
                return "bool";
            if(std::holds_alternative<std::string>(v))
                return "string";
            return "clock";
        }

        auto value_to_json(const expr::symbol_value_t& value) -> nlohmann::json {
            const auto& v = base(value);
            if(const auto* i = std::get_if<int>(&v))
                return *i;
            if(const auto* f = std::get_if<float>(&v)) {
                // NOTE: the shortest text that is the float (0.1f is 0.1, not 0.10000000149011612)
                char buffer[64]{};
                auto [end, error] = std::to_chars(buffer, buffer + sizeof(buffer), *f);
                return error == std::errc{} ? std::stod(std::string{buffer, end}) : static_cast<double>(*f);
            }
            if(const auto* b = std::get_if<bool>(&v))
                return *b;
            if(const auto* s = std::get_if<std::string>(&v))
                return *s;
            return std::get<expr::clock_t>(v).time_units;
        }

        // The value of a variable that is now the current value, from json (std::nullopt if it cannot take it)
        auto value_from_json(const nlohmann::json& json, const expr::symbol_value_t& current) -> std::optional<expr::symbol_value_t> {
            const auto& v = base(current);
            if(std::holds_alternative<int>(v) && json.is_number_integer()) {
                auto value = json.get<long long>();
                if(value < INT_MIN || value > INT_MAX)
                    return std::nullopt;
                return expr::symbol_value_t{static_cast<int>(value)};
            }
            if(std::holds_alternative<float>(v) && json.is_number())
                return expr::symbol_value_t{static_cast<float>(json.get<double>())};
            if(std::holds_alternative<bool>(v) && json.is_boolean())
                return expr::symbol_value_t{json.get<bool>()};
            if(std::holds_alternative<std::string>(v) && json.is_string())
                return expr::symbol_value_t{json.get<std::string>()};
            if(std::holds_alternative<expr::clock_t>(v) && json.is_number_integer() && json.get<long long>() >= 0 &&
               json.get<long long>() <= UINT_MAX) {
                expr::clock_t clock{};
                clock.time_units = static_cast<unsigned int>(json.get<long long>());
                return expr::symbol_value_t{clock};
            }
            return std::nullopt;
        }

        auto variable_json(const expr::symbol_value_t& value) -> nlohmann::json {
            return {{"value", value_to_json(value)}, {"type", type_of(value)}, {"writable", true}};
        }

        auto string_member(const nlohmann::json& object, const char* key, const std::string& context) -> std::string {
            auto it = object.find(key);
            if(it == object.end() || it->is_null())
                return {};
            if(!it->is_string())
                throw request_error(invalid_params, context + ": '" + key + "' must be a string");
            return it->get<std::string>();
        }

        // The model and the element of a breakpoint, {"element": {"model": "lamp.json", "id": "v"}} (both empty if the
        // breakpoint has no element)
        auto element_from_json(const nlohmann::json& breakpoint, const std::string& context) -> std::pair<std::string, std::string> {
            auto element = breakpoint.find("element");
            if(element == breakpoint.end() || element->is_null())
                return {};
            if(!element->is_object())
                throw request_error(invalid_params, context + ": 'element' must be an object");
            auto model = string_member(*element, "model", context);
            auto id = string_member(*element, "id", context);
            if(model.empty() || id.empty())
                throw request_error(invalid_params, context + ": the element needs a model and an id");
            return {model, id};
        }

        // The hit count of a breakpoint, {"hitCount": 3} (0 if it has none)
        auto hit_count_from_json(const nlohmann::json& breakpoint, const std::string& context) -> int {
            auto hits = breakpoint.find("hitCount");
            if(hits == breakpoint.end() || hits->is_null())
                return 0;
            if(!hits->is_number_integer() || hits->get<long long>() < 0 || hits->get<long long>() > INT_MAX)
                throw request_error(invalid_params, context + ": 'hitCount' must be a whole number that is not negative");
            return hits->get<int>();
        }

        // why evaluating or computing failed, for the user
        auto failure(const std::exception& e) -> std::string {
            if(dynamic_cast<const std::bad_variant_access*>(&e) != nullptr)
                return "an expression has a value of the wrong type (e.g. a guard that is not boolean)";
            return e.what();
        }
    }

    // A breakpoint of the debug protocol. For example, a breakpoint on the vertex "v" of the model "lamp.json" that only
    // counts for the instance "Main.lamp(1)", only while x > 1 holds, and is hit the third time it counts:
    //   {
    //     "id": "b1",
    //     "element": {"model": "lamp.json", "id": "v"},  (optional: without it, the breakpoint is only a condition)
    //     "instance": "Main.lamp(1)",                    (optional)
    //     "condition": "x > 1",                          (optional, unless there is no element)
    //     "hitCount": 3                                  (optional)
    //   }
    auto breakpoint_from_json(const nlohmann::json& json) -> breakpoint_t {
        if(!json.is_object())
            throw request_error(invalid_params, "a breakpoint must be an object");

        // the id, which names the breakpoint in the errors
        breakpoint_t result{};
        result.id = string_member(json, "id", "breakpoint");
        if(result.id.empty())
            throw request_error(invalid_params, "a breakpoint needs an id");
        auto context = "breakpoint " + result.id;

        // where the breakpoint is
        auto [model, element] = element_from_json(json, context);
        result.model = model;
        result.element = element;
        result.instance = string_member(json, "instance", context);

        // when it is hit
        result.condition = string_member(json, "condition", context);
        result.hit_count = hit_count_from_json(json, context);
        if(result.element.empty() && result.condition.empty())
            throw request_error(invalid_params, context + ": a breakpoint needs an element or a condition");
        return result;
    }

    auto to_json(const breakpoint_status_t& status) -> nlohmann::json {
        nlohmann::json result{{"id", status.id}, {"verified", status.verified}};
        if(!status.message.empty())
            result["message"] = status.message;
        return result;
    }

    auto to_string(stop_reason reason) -> std::string {
        switch(reason) {
            case stop_reason::entry: return "entry";
            case stop_reason::step: return "step";
            case stop_reason::breakpoint: return "breakpoint";
            case stop_reason::pause: return "pause";
            case stop_reason::stuck: return "stuck";
            case stop_reason::error: return "error";
        }
        return "error";
    }

    auto arguments_from_json(const nlohmann::json& json) -> arguments_t {
        arguments_t result{};
        if(json.is_null())
            return result;
        if(!json.is_object())
            throw request_error(invalid_params, "the arguments of the configuration must be an object");
        if(auto policy = json.find("policy"); policy != json.end()) {
            if(*policy == "first")
                result.policy = policy_t::first;
            else if(*policy == "random")
                result.policy = policy_t::random;
            else
                throw request_error(invalid_params, "the argument 'policy' must be \"first\" or \"random\"");
        }
        auto whole = [&json](const char* key, long long minimum, long long maximum) -> std::optional<long long> {
            auto it = json.find(key);
            if(it == json.end() || it->is_null())
                return std::nullopt;
            if(!it->is_number_integer() || it->get<long long>() < minimum || it->get<long long>() > maximum)
                throw request_error(invalid_params, "the argument '" + std::string{key} + "' must be a whole number from " +
                                                    std::to_string(minimum) + " to " + std::to_string(maximum));
            return it->get<long long>();
        };
        if(auto seed = whole("seed", 0, UINT_MAX))
            result.seed = static_cast<unsigned int>(*seed);
        if(auto delay = whole("delay", 0, 3600000))
            result.delay = static_cast<int>(*delay);
        if(auto history = whole("history", 1, 1000000))
            result.history = static_cast<std::size_t>(*history);
        return result;
    }

    runtime::runtime(graphedit::network_t network, arguments_t arguments)
            : network_{std::move(network)}, arguments_{arguments},
              random_{arguments.seed.has_value() ? arguments.seed.value() : std::random_device{}()} {
        if(!network_.automata)
            throw std::invalid_argument("aaltitoad could not build the network");
        for(auto& [name, instance] : network_.instances)
            for(auto& local : instance.local_identifiers)
                private_symbols_[name + "." + local] = {name, local};
    }

    auto runtime::arguments() const -> const arguments_t& {
        return arguments_;
    }

    auto runtime::network() const -> const graphedit::network_t& {
        return network_;
    }

    auto runtime::set_breakpoints(const std::vector<breakpoint_t>& breakpoints) -> std::vector<breakpoint_status_t> {
        std::vector<active_breakpoint_t> result{};
        std::vector<breakpoint_status_t> statuses{};
        for(auto& definition : breakpoints) {
            active_breakpoint_t breakpoint{.definition=definition};
            auto problem = compile(breakpoint);
            breakpoint.verified = !problem.has_value();
            breakpoint.message = problem.value_or("");
            // NOTE: hits are counted from the moment a breakpoint was set
            for(auto& old : breakpoints_)
                if(old.definition == definition)
                    breakpoint.hits = old.hits;
            statuses.push_back({definition.id, breakpoint.verified, breakpoint.message});
            result.push_back(std::move(breakpoint));
        }
        breakpoints_ = std::move(result);
        update_held();
        return statuses;
    }

    auto runtime::compile(active_breakpoint_t& breakpoint) -> std::optional<std::string> {
        const auto& definition = breakpoint.definition;
        const graphedit::instance_t* instance = nullptr;
        if(!definition.instance.empty()) {
            auto it = network_.instances.find(definition.instance);
            if(it == network_.instances.end())
                return "the network has no instance '" + definition.instance + "'";
            instance = &it->second;
        }

        // a breakpoint without an element is a condition in the scope of its instance (or the global scope)
        if(definition.element.empty())
            return compile_condition(breakpoint, {instance});

        if(auto problem = locate(breakpoint))
            return problem;
        if(instance != nullptr && instance->model != definition.model)
            return "'" + definition.instance + "' is not an instance of '" + definition.model + "'";

        // the condition of a breakpoint on an element counts in its instance, or in every instance of its model
        auto scopes = network_.instances_of(definition.model);
        if(scopes.empty())
            return "the network does not instantiate '" + definition.model + "'";
        if(instance != nullptr)
            scopes = {instance};
        return compile_condition(breakpoint, scopes);
    }

    auto runtime::locate(active_breakpoint_t& breakpoint) const -> std::optional<std::string> {
        const auto& definition = breakpoint.definition;
        auto model = network_.models.find(definition.model);
        if(model == network_.models.end())
            return "'" + definition.model + "' is not a model of the network";

        if(model->second.find_location(definition.element) != nullptr) {
            breakpoint.locations.insert(definition.element);
            return std::nullopt;
        }
        if(model->second.find_subcomponent(definition.element) != nullptr)
            return "breakpoints on subcomponents are not supported: put them on the locations and edges of the template";

        // the element is a part of edges (a connection or a nail)
        for(auto* edge : model->second.edges_through(definition.element))
            breakpoint.edges.insert(edge->id);
        if(breakpoint.edges.empty())
            return "the element is not a location, and no edge of the template passes through it";
        return std::nullopt;
    }

    auto runtime::compile_condition(active_breakpoint_t& breakpoint, const std::vector<const graphedit::instance_t*>& scopes) -> std::optional<std::string> {
        const auto& definition = breakpoint.definition;
        if(definition.condition.empty())
            return std::nullopt;

        const auto& automata = *network_.automata;
        for(auto* scope : scopes) {
            // the identifiers of the condition are those of the scope (e.g. "on" is "Main.lamp(1).on")
            auto parsed = graphedit::parse_with([scope, &definition]() {
                if(scope == nullptr)
                    return hawk::scoped_compiler{{}, {}, "", {}}.parse(definition.condition);
                return hawk::scoped_compiler{scope->local_identifiers, scope->parameters, scope->name + ".", {}}.parse(definition.condition);
            });
            if(!parsed.result)
                return "the condition is not valid: " + parsed.error;
            if(!parsed.result->expression || !parsed.result->declarations.empty())
                return "the condition is not an expression";

            for(auto& identifier : graphedit::identifiers_of(parsed.result->expression.value()))
                if(!automata.symbols.contains(identifier) && !automata.external_symbols.contains(identifier))
                    return "'" + identifier + "' is not declared";

            // the condition must be a boolean (evaluated in the current state)
            try {
                auto value = expr::evaluator{{automata.symbols, automata.external_symbols}, expr::symbol_operator{}}
                        .evaluate(parsed.result->expression.value());
                if(!std::holds_alternative<bool>(base(value)))
                    return "the condition is not a boolean expression";
            } catch(const std::exception& e) {
                return "the condition cannot be evaluated: " + failure(e);
            }

            breakpoint.conditions[scope == nullptr ? "" : scope->name] = parsed.result->expression.value();
        }
        return std::nullopt;
    }

    auto runtime::holds(const active_breakpoint_t& breakpoint, const std::string& scope) const -> bool {
        auto condition = breakpoint.conditions.find(scope);
        if(condition == breakpoint.conditions.end())
            return false;
        try {
            const auto& automata = *network_.automata;
            auto value = expr::evaluator{{automata.symbols, automata.external_symbols}, expr::symbol_operator{}}.evaluate(condition->second);
            const auto* result = std::get_if<bool>(&base(value));
            return result != nullptr && *result;
        } catch(const std::exception&) {
            return false;
        }
    }

    void runtime::update_held() {
        for(auto& breakpoint : breakpoints_)
            if(breakpoint.verified && breakpoint.definition.element.empty())
                breakpoint.held = holds(breakpoint, breakpoint.definition.instance);
    }

    auto runtime::state(bool live) -> nlohmann::json {
        if(!state_id_)
            state_id_ = "s" + std::to_string(next_state_++);
        if(!live && !history_.contains(state_id_.value())) {
            history_[state_id_.value()] = take_snapshot();
            history_order_.push_back(state_id_.value());
            while(history_order_.size() > arguments_.history) {
                history_.erase(history_order_.front());
                history_order_.pop_front();
            }
        }
        const auto& automata = *network_.automata;
        auto variables = nlohmann::json::object();
        std::map<std::string, nlohmann::json> instance_variables{};
        for(auto& [name, value] : automata.symbols) {
            auto variable = variable_of(name);
            if(variable.instance.empty())
                variables[name] = variable_json(value);
            else
                instance_variables[variable.instance][variable.name] = variable_json(value);
        }
        for(auto& [name, value] : automata.external_symbols)
            variables[name] = variable_json(value);
        auto instances = nlohmann::json::array();
        for(auto& [name, instance] : network_.instances) {
            auto variables_of_instance = instance_variables.contains(name) ? instance_variables.at(name) : nlohmann::json::object();
            instances.push_back({{"name", name}, {"model", instance.model},
                                 {"active", {automata.components.at(name).current_location->first}},
                                 {"variables", variables_of_instance}});
        }
        const auto& all = choices();
        std::string message{};
        if(!choices_error_.empty())
            message = "aaltitoad cannot compute the steps: " + choices_error_;
        else if(all.empty())
            message = stuck_description;
        else if(all.size() > max_choices)
            message = std::to_string(all.size()) + " steps are possible; the first " + std::to_string(max_choices) + " are listed";
        // NOTE: the edges that every choice takes do not tell the choices apart, so their labels leave them out
        std::set<const graphedit::instance_edge_t*> common{};
        for(std::size_t i = 0; i < all.size() && i < max_choices; i++) {
            auto edges = edges_of(all[i]);
            std::set<const graphedit::instance_edge_t*> taken{edges.begin(), edges.end()};
            if(i == 0) {
                common = taken;
                continue;
            }
            std::set<const graphedit::instance_edge_t*> shared{};
            std::set_intersection(common.begin(), common.end(), taken.begin(), taken.end(), std::inserter(shared, shared.end()));
            common = std::move(shared);
        }
        if(all.size() < 2)
            common.clear();
        // NOTE: choices whose labels are the same (e.g. edges to locations without nicknames) are told apart by the
        //       guards of their edges, or else by their ids
        std::vector<std::string> labels{};
        for(std::size_t i = 0; i < all.size() && i < max_choices; i++)
            labels.push_back(label(all[i], common));
        auto make_unique = [&labels](const std::function<std::string(std::size_t)>& relabel) {
            std::map<std::string, int> count{};
            for(auto& l : labels)
                count[l]++;
            for(std::size_t i = 0; i < labels.size(); i++)
                if(count.at(labels[i]) > 1)
                    labels[i] = relabel(i);
        };
        make_unique([this, &all, &common](std::size_t i) { return label(all[i], common, true); });
        make_unique([&labels](std::size_t i) { return labels[i] + " (c" + std::to_string(i) + ")"; });
        auto choices_json = nlohmann::json::array();
        for(std::size_t i = 0; i < all.size() && i < max_choices; i++) {
            auto elements = nlohmann::json::array();
            std::map<std::string, std::size_t> index_of{};
            for(const auto* edge : edges_of(all[i])) {
                if(!index_of.contains(edge->instance)) {
                    index_of[edge->instance] = elements.size();
                    elements.push_back({{"instance", edge->instance}, {"model", edge->model}, {"ids", nlohmann::json::array()}});
                }
                auto& ids = elements[index_of.at(edge->instance)]["ids"];
                for(auto& element : edge->edge.elements)
                    ids.push_back(element);
            }
            choices_json.push_back({{"id", "c" + std::to_string(i)}, {"label", labels[i]}, {"kind", "step"}, {"elements", elements}});
        }
        return {{"id", state_id_.value()}, {"message", message}, {"instances", instances}, {"variables", variables},
                {"choices", choices_json}};
    }

    auto runtime::step(const std::optional<std::string>& choice) -> stop_t {
        const auto& all = choices();
        std::optional<std::size_t> index{};
        if(choice) {
            std::size_t parsed{};
            auto valid = choice->size() > 1 && choice->front() == 'c';
            if(valid) {
                auto [end, error] = std::from_chars(choice->data() + 1, choice->data() + choice->size(), parsed);
                valid = error == std::errc{} && end == choice->data() + choice->size();
            }
            if(!valid || parsed >= all.size() || parsed >= max_choices)
                throw request_error(invalid_params, "there is no step '" + choice.value() + "' in the current state");
            index = parsed;
        }
        if(!choices_error_.empty())
            return {stop_reason::error, {}, "aaltitoad cannot compute the steps: " + choices_error_};
        if(all.empty())
            return {stop_reason::stuck, {}, stuck_description};
        return take(index.has_value() ? index.value() : choose());
    }

    auto runtime::run(std::size_t max_steps, std::chrono::steady_clock::time_point deadline) -> std::optional<stop_t> {
        for(std::size_t i = 0; i < max_steps; i++) {
            if(i > 0 && std::chrono::steady_clock::now() >= deadline)
                break;
            const auto& all = choices();
            if(!choices_error_.empty())
                return stop_t{stop_reason::error, {}, "aaltitoad cannot compute the steps: " + choices_error_};
            if(all.empty())
                return stop_t{stop_reason::stuck, {}, stuck_description};
            auto stop = take(choose());
            if(stop.reason != stop_reason::step)
                return stop;
        }
        return std::nullopt;
    }

    auto runtime::choose() -> std::size_t {
        const auto& all = choices();
        if(arguments_.policy == policy_t::first || all.size() < 2)
            return 0;
        return std::uniform_int_distribution<std::size_t>{0, all.size() - 1}(random_);
    }

    auto runtime::take(std::size_t choice) -> stop_t {
        auto change = choices().at(choice);
        auto description = label(change);
        try {
            network_.automata->apply(change);
        } catch(const std::exception& e) {
            return {stop_reason::error, {}, failure(e)};
        }
        changed();

        std::vector<std::string> hit_ids{};
        std::vector<std::string> descriptions{};
        auto effect = effect_of(change);
        for(auto& breakpoint : breakpoints_) {
            auto where = hit(breakpoint, effect);
            // NOTE: a breakpoint is hit from the hit_count'th time that it counts
            if(!where || ++breakpoint.hits < std::max(1, breakpoint.definition.hit_count))
                continue;
            hit_ids.push_back(breakpoint.definition.id);
            descriptions.push_back(where.value());
        }
        if(hit_ids.empty())
            return {stop_reason::step, {}, description};
        std::string text{};
        for(std::size_t i = 0; i < descriptions.size(); i++)
            text += (i == 0 ? "" : "; ") + descriptions[i];
        return {stop_reason::breakpoint, hit_ids, text};
    }

    auto runtime::effect_of(const ntta_t::state_change_t& change) const -> step_effect_t {
        step_effect_t result{};
        for(auto& location_change : change.location_changes)
            result.entered[location_change.component->first] = location_change.new_location->first;
        for(auto& identifier : change.edges)
            if(auto edge = network_.edges.find(identifier); edge != network_.edges.end())
                result.taken[edge->second.instance].push_back(&edge->second.edge);
        return result;
    }

    auto runtime::hit(active_breakpoint_t& breakpoint, const step_effect_t& effect) -> std::optional<std::string> {
        if(!breakpoint.verified)
            return std::nullopt;
        const auto& definition = breakpoint.definition;
        if(definition.element.empty())
            return condition_hit(breakpoint);
        for(auto* instance : network_.instances_of(definition.model))
            if(auto where = element_hit(breakpoint, *instance, effect))
                return where;
        return std::nullopt;
    }

    auto runtime::condition_hit(active_breakpoint_t& breakpoint) const -> std::optional<std::string> {
        const auto& definition = breakpoint.definition;
        auto held_before = breakpoint.held;
        breakpoint.held = holds(breakpoint, definition.instance);
        if(held_before || !breakpoint.held)
            return std::nullopt;
        return "Breakpoint: " + definition.condition + (definition.instance.empty() ? "" : " in " + definition.instance);
    }

    auto runtime::element_hit(const active_breakpoint_t& breakpoint, const graphedit::instance_t& instance,
                              const step_effect_t& effect) const -> std::optional<std::string> {
        const auto& definition = breakpoint.definition;
        if(!definition.instance.empty() && instance.name != definition.instance)
            return std::nullopt;
        if(!definition.condition.empty() && !holds(breakpoint, instance.name))
            return std::nullopt;

        auto entered = effect.entered.find(instance.name);
        if(entered != effect.entered.end() && breakpoint.locations.contains(entered->second))
            return "Breakpoint at " + location_name(definition.model, entered->second) + " in " + instance.name;

        auto taken = effect.taken.find(instance.name);
        if(taken == effect.taken.end())
            return std::nullopt;
        for(auto* edge : taken->second) {
            if(!breakpoint.edges.contains(edge->id))
                continue;
            return "Breakpoint at the edge from " + location_name(definition.model, edge->source) + " to " +
                   location_name(definition.model, edge->target) + " in " + instance.name;
        }
        return std::nullopt;
    }

    void runtime::set_variables(const nlohmann::json& values) {
        if(!values.is_object())
            throw request_error(invalid_params, "the values must be an object of variables and their values");
        auto& automata = *network_.automata;
        std::vector<std::tuple<expr::symbol_table_t*, std::string, expr::symbol_value_t>> changes{};
        for(auto& [name, json] : values.items()) {
            auto symbol = name;
            if(auto colon = name.rfind(':'); colon != std::string::npos) {
                // a variable of an instance: "<instance>:<variable>"
                auto instance = name.substr(0, colon);
                if(!network_.instances.contains(instance))
                    throw request_error(invalid_params, "the network has no instance '" + instance + "'");
                symbol = instance + "." + name.substr(colon + 1);
                if(!private_symbols_.contains(symbol))
                    throw request_error(invalid_params, "'" + name.substr(colon + 1) + "' is not a variable of " + instance);
            }
            expr::symbol_table_t* table = nullptr;
            if(automata.symbols.contains(symbol))
                table = &automata.symbols;
            else if(automata.external_symbols.contains(symbol))
                table = &automata.external_symbols;
            if(table == nullptr || (name.find(':') == std::string::npos && private_symbols_.contains(symbol)))
                throw request_error(invalid_params, "'" + name + "' is not a variable");
            auto value = value_from_json(json, table->at(symbol));
            if(!value)
                throw request_error(invalid_params, "'" + name + "' (" + type_of(table->at(symbol)) + ") cannot take the value " + json.dump());
            changes.emplace_back(table, symbol, value.value());
        }
        for(auto& [table, symbol, value] : changes)
            (*table)[symbol] = value;
        changed();
        update_held();
    }

    void runtime::set_active(const std::string& instance, const nlohmann::json& active) {
        auto component = network_.automata->components.find(instance);
        if(component == network_.automata->components.end())
            throw request_error(invalid_params, "the network has no instance '" + instance + "'");
        if(!active.is_array() || active.size() != 1 || !active.at(0).is_string())
            throw request_error(invalid_params, "an instance is in exactly one location: give its id");
        auto location = component->second.graph->nodes.find(active.at(0).get<std::string>());
        if(location == component->second.graph->nodes.end())
            throw request_error(invalid_params, "'" + active.at(0).get<std::string>() + "' is not a location of " + instance);
        component->second.current_location = location;
        changed();
        update_held();
    }

    void runtime::restore(const std::string& state) {
        auto snapshot = history_.find(state);
        if(snapshot == history_.end())
            throw request_error(invalid_params, "the state '" + state + "' cannot be restored");
        apply_snapshot(snapshot->second);
        changed();
        state_id_ = state;
        update_held();
    }

    void runtime::changed() {
        state_id_.reset();
        choices_.reset();
        choices_error_.clear();
    }

    auto runtime::choices() -> const std::vector<ntta_t::state_change_t>& {
        if(!choices_) {
            choices_error_.clear();
            try {
                choices_ = network_.automata->tick();
            } catch(const std::exception& e) {
                choices_ = std::vector<ntta_t::state_change_t>{};
                choices_error_ = failure(e);
            }
        }
        return choices_.value();
    }

    auto runtime::location_name(const std::string& model, const std::string& location) const -> std::string {
        const auto* found = network_.models.at(model).find_location(location);
        if(found == nullptr)
            return "a location";
        if(!found->nickname.empty())
            return "'" + found->nickname + "'";
        switch(found->kind) {
            case graphedit::location_kind_t::initial: return "the initial location";
            case graphedit::location_kind_t::final: return "the final location";
            default: return "a location";
        }
    }

    auto runtime::label(const ntta_t::state_change_t& change, const std::set<const graphedit::instance_edge_t*>& common,
                        bool detailed) const -> std::string {
        constexpr std::size_t max_expression = 40;
        auto shortened = [](const std::string& text) {
            return text.size() <= max_expression ? text : text.substr(0, max_expression - 3) + "...";
        };
        auto short_name = [this](const std::string& model, const std::string& location) -> std::string {
            const auto* found = network_.models.at(model).find_location(location);
            if(found == nullptr)
                return "?";
            if(!found->nickname.empty())
                return found->nickname;
            return found->kind == graphedit::location_kind_t::initial ? "initial"
                 : found->kind == graphedit::location_kind_t::final ? "final" : "location";
        };
        std::vector<std::string> parts{};
        std::size_t omitted = 0;
        for(const auto* edge : edges_of(change)) {
            if(common.contains(edge)) {
                omitted++;
                continue;
            }
            auto part = edge->instance + ": " + short_name(edge->model, edge->edge.source) + " \u2192 " +
                        short_name(edge->model, edge->edge.target);
            if(detailed && !edge->edge.guard.empty())
                part += " if " + shortened(edge->edge.guard);
            else if(detailed && !edge->edge.update.empty())
                part += " do " + shortened(edge->edge.update);
            parts.push_back(part);
        }
        if(parts.empty())
            return omitted > 0 ? label(change, {}, detailed) : "tick";
        std::string result{};
        for(std::size_t i = 0; i < parts.size() && i < max_label_edges; i++)
            result += (i == 0 ? "" : ", ") + parts[i];
        if(parts.size() > max_label_edges)
            result += " and " + std::to_string(parts.size() - max_label_edges) + " more";
        if(omitted == 1)
            result += " (and the edge that every step takes)";
        else if(omitted > 1)
            result += " (and the " + std::to_string(omitted) + " edges that every step takes)";
        return result;
    }

    auto runtime::edges_of(const ntta_t::state_change_t& change) const -> std::vector<const graphedit::instance_edge_t*> {
        std::vector<const graphedit::instance_edge_t*> result{};
        for(auto& identifier : change.edges)
            if(auto edge = network_.edges.find(identifier); edge != network_.edges.end())
                result.push_back(&edge->second);
        std::sort(result.begin(), result.end(), [](const auto* a, const auto* b) {
            return std::tie(a->instance, a->edge.id) < std::tie(b->instance, b->edge.id);
        });
        return result;
    }

    auto runtime::variable_of(const std::string& symbol) const -> variable_name_t {
        auto it = private_symbols_.find(symbol);
        return it == private_symbols_.end() ? variable_name_t{"", symbol} : it->second;
    }

    auto runtime::take_snapshot() const -> snapshot_t {
        snapshot_t result{};
        for(auto& [name, tta] : network_.automata->components)
            result.locations[name] = tta.current_location->first;
        result.symbols = network_.automata->symbols;
        result.external_symbols = network_.automata->external_symbols;
        return result;
    }

    void runtime::apply_snapshot(const snapshot_t& snapshot) {
        auto& automata = *network_.automata;
        for(auto& [name, location] : snapshot.locations) {
            auto& tta = automata.components.at(name);
            tta.current_location = tta.graph->nodes.find(location);
        }
        automata.symbols = snapshot.symbols;
        automata.external_symbols = snapshot.external_symbols;
    }
}
