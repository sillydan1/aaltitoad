#ifndef AALTITOAD_HAWK_MODEL_H
#define AALTITOAD_HAWK_MODEL_H
#include <optional>
#include <string>
#include <vector>

namespace aaltitoad::hawk {
    struct position {
        double x;
        double y;
    };

    // ============================================================================================================= //
    namespace scanning {
        struct vertex {
            std::string identifer;
            std::string type; // e.g. "LOCATION", "TEMPLATE_INSTANCE"
            std::vector<std::string> modifiers; // e.g. "IMMEDIATE", "FINAL"

            struct {
                std::optional<std::string> name;
                std::optional<position> position;
            } debug;
        };

        struct edge {
            std::string identifier;
            std::string source;
            std::optional<std::string> guard;
            std::optional<std::string> update;
            std::string target;

            struct {
                std::optional<std::string> name;
            } debug;
        };

        struct template_t {
            std::string identifier;
            std::string signature;
            std::vector<std::string> declarations;
            std::vector<vertex> vertices;
            std::vector<edge> edges;
            std::vector<std::string> modifiers; // e.g. "MAIN"

            struct {
                std::optional<std::string> name;
                std::optional<std::string> filepath;
            } debug;
        };
    }

    // ============================================================================================================= //
    namespace parsing {
        enum class location_modifier {
            IMMEDATE, // Immediate in terms of immediacy
            INITIAL, // The location is an initial location in the template
            FINAL // The location is a final location in the template
        };

        enum class template_modifier {
            MAIN // The template is the main template - only one per network is allowed
        };

        enum class vertex_type {
            LOCATION, // Semantically signifigant locations
            INTERMEDIATE, // Nails, joints, forks, comments, etc.
            TEMPLATE_INSTANCE // Instantiations - incoming/outgoing edges determine the composition
        };

        struct location {
            std::string identifier;
            std::vector<location_modifier> modifiers;

            struct {
                std::optional<std::string> name;
                std::optional<position> position;
            } debug;
        };

        struct instantiation {
            std::string identifier;
            std::string template_identifier;
            std::string instatiation_expression;

            struct {
                std::optional<std::string> name;
                std::optional<position> position;
            } debug;
        };

        struct edge {
            std::string identifier;
            std::string source;
            std::optional<std::string> guard; // TODO: should be expr compiled tree
            std::optional<std::string> update;
            std::string target;

            struct {
                std::optional<std::string> name;
            } debug;
        };

        struct template_t {
            std::string identifier;
            std::string signature;
            std::vector<std::string> declarations; // TODO: should be expr compiled trees
            std::vector<location> locations;
            std::vector<edge> edges;
            std::vector<template_modifier> modifiers;

            struct {
                std::optional<std::string> name;
                std::optional<std::string> filepath;
            } debug;
        };
    }
}

#endif // AALTITOAD_HAWK_MODEL_H
