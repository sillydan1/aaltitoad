#ifndef AALTITOAD_TTA_H
#define AALTITOAD_TTA_H
#include <string>
#include <graph>
#include <drivers/compiler.h>
#include <drivers/interpreter.h>
#include <symbol_table.h>
#include <hashcombine>
#include <utility>
#include <uuid>
#include <permutation>
#include <set>
#include <future>

namespace aaltitoad {
    struct location_t {
        using graph_key_t = std::string;
        std::string identifier{ya::uuid_v4_custom("L", "")};
    };

    struct edge_t {
        std::string identifier{ya::uuid_v4_custom("E", "")};
        expr::compiler::compiled_expr_t guard{};
        expr::compiler::compiled_expr_collection_t updates{};
        auto operator==(const edge_t& other) const -> bool {
            return identifier == other.identifier;
        }
    };
}

namespace std {
    template<>
    struct hash<aaltitoad::edge_t> {
        inline auto operator()(const aaltitoad::edge_t& v) const -> size_t {
            return std::hash<std::string>{}(v.identifier);
        }
    };
}

namespace aaltitoad {
    struct tta_t {
        using graph_builder = ya::graph_builder<location_t, edge_t, location_t::graph_key_t>;
        using graph_t = ya::graph<location_t, edge_t, location_t::graph_key_t>;
        using graph_node_iterator_t = ya::node_refference<location_t, edge_t, location_t::graph_key_t>;
        using graph_edge_iterator_t = ya::edge_refference<location_t, edge_t, location_t::graph_key_t>;
        std::shared_ptr<graph_t> graph;
        location_t::graph_key_t initial_location;
        graph_node_iterator_t current_location;

        tta_t() : graph{}, initial_location{}, current_location{} {}
        tta_t(std::shared_ptr<graph_t> graph, location_t::graph_key_t initial_location)
                : graph{std::move(graph)}, initial_location{std::move(initial_location)},
                  current_location{}
        {
            auto it = this->graph->nodes.find(this->initial_location);
            if(it == this->graph->nodes.end())
                throw std::out_of_range("no such initial location in provided TTA graph");
            current_location = it;
        }
    };

    struct tocker_t;

    struct ntta_t {
#ifndef NDEBUG
        using tta_map_t = std::map<std::string,tta_t>;
#else
        using tta_map_t = std::unordered_map<std::string,tta_t>;
#endif
        struct location_change_t {
            tta_map_t::iterator component;
            tta_t::graph_node_iterator_t new_location;
        };
        struct choice_t {
            tta_t::graph_edge_iterator_t edge;
            location_change_t location_change;
            expr::symbol_table_t symbol_changes;
        };
        struct state_change_t {
            std::vector<location_change_t> location_changes;
            expr::symbol_table_t symbol_changes;
            auto operator+=(const choice_t&) -> state_change_t&;
        };

        std::vector<std::unique_ptr<tocker_t>> tockers;
        expr::symbol_table_t symbols;
        expr::symbol_table_t external_symbols;
        tta_map_t components;

        ntta_t() : symbols{}, external_symbols{}, components{} {}
        ntta_t(expr::symbol_table_t symbols, tta_map_t components)
         : symbols{std::move(symbols)}, external_symbols{}, components{std::move(components)} {}
        ntta_t(expr::symbol_table_t symbols, expr::symbol_table_t external_symbols, tta_map_t components)
         : symbols{std::move(symbols)}, external_symbols{std::move(external_symbols)}, components{std::move(components)} {}

        auto tick() -> std::vector<state_change_t>;
        auto tock() const -> std::vector<expr::symbol_table_t>;
        void apply(const state_change_t& changes);
        void apply(const expr::symbol_table_t& symbol_changes);
    private:
        class tick_resolver {
        public:
            using set = std::set<std::string>;
            using solution_keys = std::vector<std::set<std::string>>;
            using graph_type = ya::graph<tta_t::graph_edge_iterator_t, uint32_t, std::string>;
            using graph_type_builder = ya::graph_builder<tta_t::graph_edge_iterator_t, uint32_t, std::string>;
            struct choice_dependency_problem {
                graph_type dependency_graph;
                std::unordered_map<std::string, choice_t> choices;
            };

            explicit tick_resolver(const graph_type& G);
            auto solve() -> solution_keys;
        private:
            void solve_recursive(solution_keys& S, const set& a);
            auto get_neighbors(const std::string& node_key) -> set;
            auto get_postsets(const set& node_keys) -> set;
            static auto _union(const set& a, const set& b) -> set;
            static auto _difference(const set& a, const set& b) -> set;

            const graph_type& G;
            set N;
        };
        auto generate_enabled_choice_dependency_graph() -> tick_resolver::choice_dependency_problem;
        static auto eval_updates(expr::interpreter& i, const expr::compiler::compiled_expr_collection_t& t) -> expr::symbol_table_t;
        static auto eval_guard(expr::interpreter& i, const expr::compiler::compiled_expr_t& e) -> expr::symbol_value_t;
    };

    struct tocker_t {
        [[nodiscard]] virtual auto tock(const ntta_t& state) const -> std::vector<expr::symbol_table_t> = 0;
        virtual ~tocker_t() = default;
    };

    class async_tocker_t : public tocker_t {
    protected:
        mutable std::future<expr::symbol_table_t> job{};
        virtual expr::symbol_table_t get_tock_values(const expr::symbol_table_t& invocation_environment) const = 0;
        virtual void tock_async(const expr::symbol_table_t& environment) const {
            job = std::async([this, &environment](){
                return get_tock_values(environment);
            });
        }
        ~async_tocker_t() override = default;

    public:
        auto tock(const ntta_t& state) const -> std::vector<expr::symbol_table_t> override {
            if(!job.valid())
                return {};
            auto c = job.get();
            tock_async(state.symbols);
            return {c};
        }
    };
}

auto operator<<(std::ostream& os, const aaltitoad::ntta_t& state) -> std::ostream&;

namespace std {
    template<>
    struct hash<aaltitoad::tta_t> {
        inline auto operator()(const aaltitoad::tta_t& v) const -> size_t {
            return std::hash<aaltitoad::location_t::graph_key_t>{}(v.current_location->first);
        }
    };

    template<>
    struct hash<aaltitoad::ntta_t::tta_map_t> {
        inline auto operator()(const aaltitoad::ntta_t::tta_map_t& v) const -> size_t {
            size_t result{};
            for(auto& t : v) {
                result = ya::hash_combine(result, t.first);
                result = ya::hash_combine(result, t.second);
            }
            return result;
        }
    };

    template<>
    struct hash<aaltitoad::ntta_t> {
        inline auto operator()(const aaltitoad::ntta_t& v) const -> size_t {
            return ya::hash_combine(v.symbols, v.components);
        }
    };

    template<>
    struct hash<aaltitoad::tta_t::graph_edge_iterator_t> {
        inline auto operator()(const aaltitoad::tta_t::graph_edge_iterator_t& v) const -> size_t {
            return hash<std::string>{}(v->second.data.identifier);
        }
    };
}

#endif //AALTITOAD_TTA_H
