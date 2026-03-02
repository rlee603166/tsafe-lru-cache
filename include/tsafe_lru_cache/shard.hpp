#pragma once

#include <memory>
#include <optional>
#include <vector>
#include "tsafe_cache.hpp"

namespace sharded_cache {

static constexpr size_t NUM_SHARDS = 16;

template <typename K, typename V>
class ShardedLRUCache {
private:
    std::vector<std::unique_ptr<tsafe_cache::TSafeCache<K, V>>> shards_;

    tsafe_cache::TSafeCache<K, V>& getShard(const K& key);

public:
    explicit ShardedLRUCache(int total_capacity = 160);

    std::optional<V> get(const K& key);
    void put(const K& key, const V& value);
    bool contains(const K& key);
    void clear();
    int getSize();
    int getCapacity();
    bool empty();
    bool full();
};

} // namespace sharded_cache

#include "shard.tpp"
