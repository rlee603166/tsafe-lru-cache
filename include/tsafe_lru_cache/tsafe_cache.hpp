#pragma once

#include <optional>
#include <shared_mutex>
#include "lru_cache/lru_cache.hpp"

namespace tsafe_cache {

template <typename T1, typename T2>
class TSafeCache {
private:
    mutable std::shared_mutex mtx_;
    lru_cache::LRUCache<T1, T2>* cache_;

public:
    TSafeCache(int cap = 10);
    void put(const T1& key, const T2& value);
    std::optional<T2> get(const T1& key);
    bool contains(const T1& key);
    void clear();
    int getCapacity();
    int getSize();
    bool empty();
    bool full();
    void printCache();
};

} // namespace tsafe_cache

#include "tsafe_cache.tpp"
