#ifndef AALTITOAD_FUNCTION_LAYER_H
#define AALTITOAD_FUNCTION_LAYER_H
#include <stack>
#include <utility>

template<typename T>
class value_layer {
public:
    explicit value_layer(std::string layer_name) : debug_name{std::move(layer_name)} {}
    virtual ~value_layer() = default;
    virtual void on_attach(const T&) {}
    virtual void on_detach(const T&) {}
    virtual T on_call(const T& t) { return t; }
    std::string get_name() const { return debug_name; }
protected:
    std::string debug_name;
};

template<typename T>
class value_layer_stack {
public:
    value_layer_stack() : collection{}, environment{}, on_apply_callback{} {}
    template<typename F>
    explicit value_layer_stack(F callback) : collection{}, environment{}, on_apply_callback{callback} {}
    ~value_layer_stack() {
        for(auto& l : collection)
            l->on_detach(environment);
    }
    template<typename L, typename... Ts>
    void add_layer(Ts&&... ts) {
        collection.push_back(std::make_unique<L>(std::forward<Ts>(ts)...));
        collection.back()->on_attach(environment);
    }
    T apply() {
        for(auto& l : collection) {
            on_apply_callback(l->get_name());
            environment = l->on_call(environment);
        }
        return environment;
    }
private:
    std::vector<std::unique_ptr<value_layer<T>>> collection;
    T environment;
    std::function<void(const std::string&)> on_apply_callback;
};

#endif //AALTITOAD_FUNCTION_LAYER_H
