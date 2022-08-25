#ifndef AALTITOAD_FORWARD_REACHABILITY_H
#define AALTITOAD_FORWARD_REACHABILITY_H
#include "ctl_sat.h"
#include "ntta/tta.h"
#include <unordered_map>
#include <map>
#include <extensions/random.h>
#include <extensions/exceptions/not_implemented_yet_exception.h>
#include <ctl_compiler.h>

namespace aaltitoad {
    enum class pick_strategy {
        first, last, random
    };

    template<typename T>
    class colliding_multimap {
        // TODO: static check that std::hash<T> exists
        std::multimap<size_t, T> data{};
    public:
        colliding_multimap(std::initializer_list<T> ts) : data{} {
            for(auto t : ts)
                add(t);
        }
        auto empty() -> bool {
            return data.empty();
        }
        void add(const T& v) {
            add(std::hash<T>{}(v), v);
        }
        void add(size_t key, const T& v) {
            data.insert({key, v});
        }
        void add_if_other_does_not_contains(const T& v, const colliding_multimap& other) {
            if(!other.contains(v))
                add(v);
        }
        auto contains(const T& v) const -> bool {
            return contains(std::hash<T>{}(v), v);
        }
        auto contains(size_t key, const T& v) const -> bool {
            auto range = data.equal_range(key);
            for(auto& it = range.first; it != range.second; it++)
                if(it->second == v)
                    return true;
            return false;
        }
        auto pop(const pick_strategy& strategy = pick_strategy::first) -> T {
            switch (strategy) {
                case pick_strategy::first: {
                    auto r = data.begin()->second;
                    data.erase(data.begin());
                    return r;
                }
                case pick_strategy::last: {
                    auto r = data.end()->second;
                    data.erase(data.end());
                    return r;
                }
                case pick_strategy::random: {
                    auto pick = random::value(0, data.size() - 1);
                    auto it = data.begin();
                    for (auto i = 0; i < pick; i++)
                        it++;
                    auto r = it->second;
                    data.erase(it);
                    return r;
                }
                default:
                    throw not_implemented_yet_exception();
            }
        }
    };

    class forward_reachability_searcher {
        colliding_multimap<ntta_t> W, P;
    public:
        auto is_reachable(const ntta_t& model, const ctl::compiler::compiled_expr_t& q) -> bool;
    };
}

#endif //AALTITOAD_FORWARD_REACHABILITY_H
