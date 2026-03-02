#pragma once

#include <atomic>
#include <optional>
#include <shared_mutex>
#include <unordered_map>
#include <vector>

#include "bloom_filter.hpp"

namespace clock_cache {

template <typename K, typename V>
class ClockCache {
private:
    struct Entry {
        K key;
        V value;
        std::atomic<bool> ref_bit{false};
        bool occupied{false};
    };

    std::vector<Entry> buffer_;
    std::unordered_map<K, size_t> hashmap_;
    size_t clock_hand_;
    int capacity_;
    mutable std::shared_mutex mtx_;
    bloom::BloomFilter<K> bloom_;

    size_t evict();

public:
    explicit ClockCache(int cap = 10);

    std::optional<V> get(const K& key);
    void put(const K& key, const V& value);
    bool contains(const K& key);
    void clear();
    int getSize();
    int getCapacity();
    bool empty();
    bool full();
};

} // namespace clock_cache

#include "clock_cache.tpp"
