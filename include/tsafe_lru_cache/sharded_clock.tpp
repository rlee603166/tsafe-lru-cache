#pragma once

#include <algorithm>

namespace sharded_clock {

template <typename K, typename V>
ShardedClockCache<K, V>::ShardedClockCache(int total_capacity) {
    int per_shard = std::max(1, total_capacity / static_cast<int>(NUM_SHARDS));
    shards_.reserve(NUM_SHARDS);
    for (size_t i = 0; i < NUM_SHARDS; ++i) {
        shards_.push_back(std::make_unique<clock_cache::ClockCache<K, V>>(per_shard));
    }
}

template <typename K, typename V>
clock_cache::ClockCache<K, V>& ShardedClockCache<K, V>::getShard(const K& key) {
    size_t idx = std::hash<K>{}(key) % NUM_SHARDS;
    return *shards_[idx];
}

template <typename K, typename V>
std::optional<V> ShardedClockCache<K, V>::get(const K& key) {
    return getShard(key).get(key);
}

template <typename K, typename V>
void ShardedClockCache<K, V>::put(const K& key, const V& value) {
    getShard(key).put(key, value);
}

template <typename K, typename V>
bool ShardedClockCache<K, V>::contains(const K& key) {
    return getShard(key).contains(key);
}

template <typename K, typename V>
void ShardedClockCache<K, V>::clear() {
    for (auto& shard : shards_) {
        shard->clear();
    }
}

template <typename K, typename V>
int ShardedClockCache<K, V>::getSize() {
    int total = 0;
    for (auto& shard : shards_) {
        total += shard->getSize();
    }
    return total;
}

template <typename K, typename V>
int ShardedClockCache<K, V>::getCapacity() {
    int total = 0;
    for (auto& shard : shards_) {
        total += shard->getCapacity();
    }
    return total;
}

template <typename K, typename V>
bool ShardedClockCache<K, V>::empty() {
    return getSize() == 0;
}

template <typename K, typename V>
bool ShardedClockCache<K, V>::full() {
    return getSize() == getCapacity();
}

} // namespace sharded_clock
