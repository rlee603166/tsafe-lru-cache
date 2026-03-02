#pragma once

#include <memory>
#include <optional>
#include <vector>
#include "clock_cache.hpp"

namespace sharded_clock {

static constexpr size_t NUM_SHARDS = 16;

template <typename K, typename V>
class ShardedClockCache {
private:
    std::vector<std::unique_ptr<clock_cache::ClockCache<K, V>>> shards_;

    clock_cache::ClockCache<K, V>& getShard(const K& key);

public:
    explicit ShardedClockCache(int total_capacity = 160);

    std::optional<V> get(const K& key);
    void put(const K& key, const V& value);
    bool contains(const K& key);
    void clear();
    int getSize();
    int getCapacity();
    bool empty();
    bool full();
};

} // namespace sharded_clock

#include "sharded_clock.tpp"
