#pragma once
#include <any>
#include <map>
#include <typeindex>
#include <utility>

namespace civ5::runtime {
// Copyable state or decode context owned by a GameCore compatibility module.
// The common runtime knows neither the module's fields nor their wire format.
struct ProfileExtensions {
    std::map<std::type_index, std::any> values;

    template<class T> T *find() {
        const auto it=values.find(std::type_index(typeid(T)));
        return it==values.end()?nullptr:std::any_cast<T>(&it->second);
    }
    template<class T> const T *find() const {
        const auto it=values.find(std::type_index(typeid(T)));
        return it==values.end()?nullptr:std::any_cast<T>(&it->second);
    }
    template<class T, class... Args> T &emplace(Args&&... args) {
        return std::any_cast<T&>(values.insert_or_assign(std::type_index(typeid(T)),
            std::any(T(std::forward<Args>(args)...))).first->second);
    }
};
} // namespace civ5::runtime
