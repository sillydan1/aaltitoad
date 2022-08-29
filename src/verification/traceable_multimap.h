#ifndef AALTITOAD_TRACEABLE_MULTIMAP_H
#define AALTITOAD_TRACEABLE_MULTIMAP_H
#include <map>
#include "pick_strategy.h"
#include "extensions/exceptions/not_implemented_yet_exception.h"
#include "extensions/random.h"

namespace aaltitoad {
    template<typename T>
    struct with_parent_t {
        using parent_t = typename std::multimap<size_t, with_parent_t<T>>::iterator;
        std::optional<parent_t> parent;
        T data;
    };

    template<typename T>
    class traceable_multimap {
        std::multimap<size_t, with_parent_t<T>> data{};
    public:
        using iterator_t = typename std::multimap<size_t, with_parent_t<T>>::iterator;
        traceable_multimap(std::initializer_list<T> ts) : data{} {
            for(auto t : ts)
                add(t);
        }
        auto begin() {
            return data.begin();
        }
        auto end() {
            return data.end();
        }
        auto empty() -> bool {
            return data.empty();
        }
        auto add(const T& v) {
            return add(std::hash<T>{}(v), {}, v);
        }
        auto add(const std::optional<iterator_t>& parent, const T& v) {
            return add(std::hash<T>{}(v), parent, v);
        }
        auto add(size_t key, const std::optional<iterator_t>& parent, const T& v) {
            return data.insert({key, {parent, v}});
        }
        auto contains(const T& v) const -> bool {
            return contains(std::hash<T>{}(v), v);
        }
        auto contains(size_t key, const T& v) const -> bool {
            auto range = data.equal_range(key);
            for(auto& it = range.first; it != range.second; it++)
                if(it->second.data == v)
                    return true;
            return false;
        }
        auto pop(const pick_strategy& strategy = pick_strategy::first) -> with_parent_t<T> {
            switch (strategy) {
                case pick_strategy::first:  return pop_it(data.begin());
                case pick_strategy::last:   return pop_it(last_it());
                case pick_strategy::random: return pop_it(random_it());
                default:
                    throw not_implemented_yet_exception();
            }
        }
        auto pop_it(const iterator_t& it) -> with_parent_t<T> {
            auto r = it->second;
            data.erase(it);
            return r;
        }
        auto random_it() -> iterator_t {
            auto pick = random::value(0, data.size() - 1);
            auto it = data.begin();
            for (auto i = 0; i < pick; i++, it++);
            return it;
        }
        auto last_it() -> iterator_t {
            auto it = data.begin();
            for(auto i = 0; i < data.size()-1; i++, it++);
            return it;
        }
        auto size() -> size_t {
            return data.size();
        }
    };
}

#endif //AALTITOAD_TRACEABLE_MULTIMAP_H
