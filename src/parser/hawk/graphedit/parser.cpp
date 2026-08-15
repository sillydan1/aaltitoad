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
#include "lsp.pb.h"
#include "parser.h"
#include "parser/diagnostics.h"
#include "parser/hawk/scoped_template_builder/scoped_template_builder.h"
#include "plugin_system/plugin_system.h"
#include "util/string_extensions.h"
#include <implementation/timer.h>

namespace aaltitoad::hawk::graphedit {
    static diagnostic invalid_edge_source{
        .identifier="invalid_edge_source",
        .title="Bad edge",
        .message="Invalid source",
        .description="The affected elements are edges that does not have valid source values",
        .severity=Severity::SEVERITY_ERROR};
    static diagnostic invalid_edge_target{
        .identifier="invalid_edge_target",
        .title="Bad edge",
        .message="Invalid target",
        .description="The affected elements are edges that does not have valid target values",
        .severity=Severity::SEVERITY_ERROR};
    static diagnostic empty_nail_preset{
        .identifier="empty_nail_preset",
        .title="No ingoing edges from nail",
        .message="Invalid nail",
        .description="Nails are invalid if there are no ingoing edges",
        .severity=Severity::SEVERITY_ERROR};
    static diagnostic empty_nail_postset{
        .identifier="empty_nail_postset",
        .title="No outgoing edges from nail",
        .message="Invalid nail",
        .description="Nails are invalid if there are no outgoing edges",
        .severity=Severity::SEVERITY_ERROR};
    static diagnostic multiple_nail_postset{
        .identifier="multiple_nail_postset",
        .title="More than one outgoing edge from nail",
        .message="Invalid nail",
        .description="Nails are invalid if there are more than one outgoing edge",
        .severity=Severity::SEVERITY_ERROR};
    diagnostic invalid_model_file(const std::string& filename, const std::string& err) {
        return {
            .identifier="invalid_model_file",
            .title="Could not load " + filename,
            .message=err,
            .description="Failed loading model file",
            .severity=Severity::SEVERITY_WARNING};
    }

    static diagnostic instantiations_not_supported{ // TODO: support instantiations
        .identifier="instantiations_not_supported",
        .title="Instantiations not supported yet",
        .message="Template instantiations are not supported yet",
        .description="This compiler does not support template instantiation yet. Please nag the developer on GitHub",
        .severity=Severity::SEVERITY_WARNING};
    static diagnostic freefloating_vertex{
        .identifier="freefloating_vertex",
        .title="Vertex not connected",
        .message="Freefloating vertex element",
        .description="Vertex is freefloating, not connected to anything in the graph.",
        .severity=Severity::SEVERITY_WARNING};
}

namespace aaltitoad::hawk::graphedit {
    parser::parser() {

    }

    auto parser::parse_files(const std::vector<std::string>& filepaths, const std::vector<std::string> &ignore_list) -> plugin::parse_result {
        hawk::scoped_template_builder builder{};
        std::vector<Diagnostic> diagnostics{};
        diagnostic_factory diag_factory{};
        for(const auto& filepath : filepaths) {
            for(const auto& entry: std::filesystem::directory_iterator(filepath)) {
                try {
                    if(should_ignore(entry, ignore_list)) {
                        spdlog::trace("ignoring file {0}", entry.path().c_str());
                        continue;
                    }
                    diag_factory.with_model_key(entry.path());
                    std::ifstream input_filestream(entry.path());
                    auto json_file = nlohmann::json::parse(input_filestream,
                            /* callback */ nullptr,
                            /* allow exceptions */ true,
                            /* ignore_comments */ true);
                    model_t model{};
                    try {
                        from_json(json_file, model);
                    } catch(std::exception& e) {
                        // Not a valid model file. Do nothing (for now)
                        continue;
                    }
                    // TODO: verify that the model key is the entry.path().
                    auto result = compile_model(model, entry.path());
                    if(!result.has_value()) {
                        diagnostics.insert(result.error().diagnostics.begin(), result.error().diagnostics.end(), diagnostics.end());
                        continue;
                    }
                    diagnostics.insert(result.value().diagnostics.begin(), result.value().diagnostics.end(), diagnostics.end());
                    builder.add_template(result.value().tta);
                } catch (std::exception& e) {
                    spdlog::error("unable to parse json file {0}: {1}", entry.path().c_str(), e.what());
                    diagnostics.push_back(diag_factory
                            .without_context()
                            .create_diagnostic(
                                invalid_model_file(entry.path().c_str(), e.what())));
                }
            }
        }
        spdlog::trace("building the ntta_t");
        auto result = builder.build();
        if(result.has_value())
            result.value().diagnostics.insert(result.value().diagnostics.end(), diagnostics.begin(), diagnostics.end());
        else
            result.error().diagnostics.insert(result.error().diagnostics.end(), diagnostics.begin(), diagnostics.end());
        return result;
    }

    auto parser::should_ignore(const std::filesystem::directory_entry& entry, const std::vector<std::string>& ignore_list) -> bool {
        return std::any_of(ignore_list.begin(), ignore_list.end(),
                           [&entry, this](const std::string& ig){ return should_ignore(entry, ig); });
    }

    auto parser::should_ignore(const std::filesystem::directory_entry& entry, const std::string& ignore_regex) -> bool {
        return std::regex_match(entry.path().c_str(), std::regex{ignore_regex});
    }

    auto parser::parse_model(const Buffer& buffer) -> plugin::parse_result {
        ya::timer<unsigned int> t{};
        spdlog::trace("parsing buffer: {}", buffer.path());
        hawk::scoped_template_builder builder{};
        auto result = compile_model(to_model(buffer), buffer.path());
        if(!result.has_value())
            return std::unexpected(plugin::parse_error{result.error().diagnostics});
        builder.add_diagnostics(result.value().diagnostics);
        builder.add_template(result.value().tta);
        // TODO: Expressions with non-existent identifier references will compile just fine.
        //       Add a check that ensures that all identifiers in all expressions are available in the tta network tree
        spdlog::debug("buffer parsing took {}ms", t.milliseconds_elapsed());
        return builder.build();
    }

    auto parser::to_graph(const Graph& g) -> graph_t {
        spdlog::debug("loading graphedit vertices");
        std::unordered_map<std::string, vertex_t> vertices{};
        vertices.reserve(g.vertices().size());
        for(auto& v : g.vertices()) {
            vertex_t r{};
            nlohmann::json::parse(v.jsonencoding()).get_to(r);
            vertices[v.id()] = r;
        }
        spdlog::debug("loading graphedit edges");
        std::unordered_map<std::string, edge_t> edges{};
        edges.reserve(g.edges().size());
        for(auto& e : g.edges()) {
            edge_t r{};
            nlohmann::json::parse(e.jsonencoding()).get_to(r);
            if(r.type != graphedit::edge_type_t::normal)
                continue;
            edges[e.id()] = r;
        }
        return graph_t{
            .declarations=g.declarations(),
            .vertices=vertices,
            .edges=edges
        };
    }

    auto parser::to_model(const Buffer& buffer) -> model_t {
        std::unordered_map<std::string, std::string> metadata{};
        metadata.reserve(buffer.metadata().size());
        for(auto& m : buffer.metadata())
            metadata[m.first] = m.second;
        return model_t{
            .metadata=metadata,
            .syntax=to_graph(buffer.graph())
        };
    }

    auto parser::compile_type(const graphedit::location_type_t& type) -> urgency_t {
        switch(type) {
            case location_type_t::normal:
            case location_type_t::initial:
            case location_type_t::final:
                return urgency_t::normal;
            case location_type_t::immediate:
                return urgency_t::urgent;
            case location_type_t::invalid:
            default:
                throw std::logic_error{"invalid location type"};
        }
    }

    using vertex_reference = std::string;
    struct target_reference {
        std::string vertex_key;
        std::string edge_key;
    };

    auto parser::compile_model(const model_t& model, const std::string& model_key) -> compile_result {
        std::vector<Diagnostic> diagnostics{};
        try {
            diagnostic_factory diag_factory{};
            diag_factory.with_model_key(model_key);
            auto push_diag = [&diagnostics, &diag_factory](const std::vector<std::string>& elements, const diagnostic& d) {
                spdlog::trace("diagnostic: {}", d.identifier);
                diagnostics.push_back(diag_factory.with_context(elements).create_diagnostic(d));
            };
            // NOTE: I know, this is ugly. I can optimize later
            std::unordered_map<vertex_reference, std::vector<target_reference>> postset{};
            std::unordered_map<vertex_reference, std::vector<target_reference>> preset{};

            ya::timer<unsigned int> t{};
            spdlog::trace("preprocessing model");
            for(auto& e : model.syntax.edges) {
                postset[e.second.source].push_back(target_reference{e.second.target, e.first});
                preset[e.second.target].push_back(target_reference{e.second.source, e.first});
                auto source = model.syntax.vertices.find(e.second.source);
                auto target = model.syntax.vertices.find(e.second.target);
                if(source == model.syntax.vertices.end())
                    push_diag({e.first}, invalid_edge_source);
                if(target == model.syntax.vertices.end())
                    push_diag({e.first}, invalid_edge_target);
            }

            std::vector<hawk::location_t> result_locations{};
            std::optional<hawk::location_t> initial_location{};
            std::optional<hawk::location_t> final_location{};
            std::vector<hawk::tta_instance_t> result_instances{};
            for(auto& vertex : model.syntax.vertices) {
                if(postset[vertex.first].empty() && preset[vertex.first].empty())
                    push_diag({vertex.first}, freefloating_vertex);
                if(std::holds_alternative<graphedit::location_t>(vertex.second)) {
                    auto l = std::get<graphedit::location_t>(vertex.second);
                    auto result = hawk::location_t{vertex.first, l.nickname, compile_type(l.type)};
                    switch(l.type) {
                        case location_type_t::initial:
                            initial_location = result;
                            break;
                        case location_type_t::final:
                            final_location = result;
                            break;
                        case location_type_t::normal:
                        case location_type_t::immediate:
                        case location_type_t::invalid:
                        default:
                            result_locations.push_back(result);
                            break;
                    }
                }
                else if(std::holds_alternative<graphedit::nail_t>(vertex.second)) {
                    if(postset[vertex.first].empty())
                        push_diag({vertex.first}, empty_nail_postset);
                    else if(preset[vertex.first].empty())
                        push_diag({vertex.first}, empty_nail_preset);
                    else if(postset[vertex.first].size() != 1)
                        push_diag({vertex.first}, multiple_nail_postset);
                }
                else if(std::holds_alternative<graphedit::instantiation_t>(vertex.second)) {
                    auto i = std::get<graphedit::instantiation_t>(vertex.second);
                    push_diag({vertex.first}, instantiations_not_supported);
                    continue; // TODO: extract template name out from invocation string
                    result_instances.push_back(hawk::tta_instance_t{vertex.first, i.template_name, i.template_name});
                }
            }
            spdlog::trace("preprocessing took {}ms", t.milliseconds_elapsed());

            spdlog::trace("generating edges"); t.start();
            std::vector<hawk::edge_t> result_edges{};
            std::vector<std::string> visisted_edges{};
            auto has_visited = [&visisted_edges](const std::string& edge){
                return std::find(visisted_edges.begin(), visisted_edges.end(), edge) != visisted_edges.end();
            };
            for(auto& edge : model.syntax.edges) {
                if(has_visited(edge.first))
                    continue;
                visisted_edges.push_back(edge.first);
                auto source_key = edge.second.source;
                auto target_key = edge.second.target;
                auto source = model.syntax.vertices.at(source_key);
                auto target = model.syntax.vertices.at(target_key);
                std::vector<std::string> edges = {edge.first};
                std::vector<std::string> guards{};
                std::vector<std::string> updates{};
                // Follow the edge until source is location or error.
                while(!std::holds_alternative<graphedit::location_t>(source)) {
                    if(std::holds_alternative<graphedit::nail_t>(source)) {
                        auto nail = std::get<graphedit::nail_t>(source);
                        switch(nail.type) {
                            case nail_type_t::guard:
                                guards.push_back(nail.expression);
                                break;
                            case nail_type_t::update:
                                updates.push_back(nail.expression);
                                break;
                            case nail_type_t::comment:
                            case nail_type_t::invalid:
                                break;
                        }
                    }
                    auto ps = preset.at(source_key);
                    if(ps.empty())
                        break;
                    auto r = ps.begin();
                    source = model.syntax.vertices.at(r->vertex_key);
                    source_key = r->vertex_key;
                    visisted_edges.push_back(r->edge_key);
                    edges.push_back(r->edge_key);
                }
                // Follow the edge until target is location or error.
                while(!std::holds_alternative<graphedit::location_t>(target)) {
                    if(std::holds_alternative<graphedit::nail_t>(target)) {
                        auto nail = std::get<graphedit::nail_t>(target);
                        switch(nail.type) {
                            case nail_type_t::guard:
                                guards.push_back(nail.expression);
                                break;
                            case nail_type_t::update:
                                updates.push_back(nail.expression);
                                break;
                            case nail_type_t::comment:
                            case nail_type_t::invalid:
                                break;
                        }
                    }
                    auto ps = postset.at(target_key);
                    if(ps.empty())
                        break;
                    auto r = ps.begin();
                    target = model.syntax.vertices.at(r->vertex_key);
                    target_key = r->vertex_key;
                    visisted_edges.push_back(r->edge_key);
                    edges.push_back(r->edge_key);
                }
                // Sort the found edges alphabetically, just to be nice
                std::sort(edges.begin(), edges.end());
                if(!std::holds_alternative<graphedit::location_t>(source))
                    push_diag(edges, invalid_edge_source);
                if(!std::holds_alternative<graphedit::location_t>(target))
                    push_diag(edges, invalid_edge_target);

                result_edges.push_back(hawk::edge_t{
                    .id=join(",", edges),
                    .source=source_key,
                    .target=target_key,
                    .guard=join(" && ", guards),
                    .update=join(";", updates)
                });
            }
            spdlog::trace("edge generation took {}ms ({} edges)", t.milliseconds_elapsed(), result_edges.size());
            auto fail_condition = false
                || !initial_location.has_value()
                || !final_location.has_value()
                || std::any_of(diagnostics.begin(), diagnostics.end(), [](const Diagnostic& d){ return d.severity() == Severity::SEVERITY_ERROR; })
                ;
            if(fail_condition) {
                spdlog::error("compilation failed");
                return std::unexpected(compile_error{
                    .diagnostics=diagnostics,
                });
            }
            auto is_main = model.metadata.contains("is_main") && lower_case(model.metadata.at("is_main")) == "true";
            auto model_name = model.metadata.contains("name") ? model.metadata.at("name") : model_key;
            return compile_ok{
                .diagnostics=diagnostics,
                .tta=tta_template{
                    .name=model_name,
                    .declarations=model.syntax.declarations,
                    .is_main=is_main,
                    .locations=result_locations,
                    .edges=result_edges,
                    .initial_location=initial_location.value(),
                    .final_location=final_location.value(),
                    .instances=result_instances // TODO: support instances
                }
            };
        } catch(std::exception& e) {
            spdlog::error("compilation error: {}", e.what());
            return std::unexpected(compile_error{
                .diagnostics=diagnostics,
            });
        }
    }
}

extern "C" {
    const char* get_plugin_name() {
        return "graphedit_parser";
    }

    const char* get_plugin_version() {
        return "v1.0.0";
    }

    plugin_type get_plugin_type() {
        return plugin_type::parser;
    }

    aaltitoad::plugin::parser* create_parser() {
        return new aaltitoad::hawk::graphedit::parser();
    }
}
