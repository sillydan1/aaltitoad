#ifndef AALTITOAD_MAP_EXTENSIONS_H
#define AALTITOAD_MAP_EXTENSIONS_H
#include <vector>
#include <unordered_map>
#include <map>

struct key_element_t {
    template <typename T>
    typename T::first_type operator()(T keyValuePair) const {
        return keyValuePair.first;
    }
};
struct value_element_t {
    template <typename T>
    typename T::first_type operator()(T keyValuePair) const {
        return keyValuePair.second;
    }
};

template<typename K, typename V>
auto get_key_set(const std::unordered_map<K,V>& m) -> std::vector<K> {
    std::vector<K> keys{};
    transform(m.begin(), m.end(), back_inserter(keys), key_element_t());
    return keys;
}
template<typename K, typename V>
auto get_key_set(const std::map<K,V>& m) -> std::vector<K> {
    std::vector<K> keys{};
    transform(m.begin(), m.end(), back_inserter(keys), key_element_t());
    return keys;
}

template<typename K, typename V>
auto get_value_set(const std::unordered_map<K,V>& m) -> std::vector<V> {
    std::vector<V> values{};
    transform(m.begin(), m.end(), back_inserter(values), value_element_t());
    return values;
}
template<typename K, typename V>
auto get_value_set(const std::map<K,V>& m) -> std::vector<V> {
    std::vector<V> values{};
    transform(m.begin(), m.end(), back_inserter(values), value_element_t());
    return values;
}

#endif
