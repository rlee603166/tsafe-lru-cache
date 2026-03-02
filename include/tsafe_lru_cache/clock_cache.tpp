#pragma once

#include <algorithm>

namespace clock_cache {

template <typename K, typename V>
ClockCache<K, V>::ClockCache(int cap)
    : buffer_(std::max(cap, 0)), clock_hand_(0), capacity_(std::max(cap, 0)),
      bloom_(std::max(cap, 0)) {}

template <typename K, typename V>
size_t ClockCache<K, V>::evict() {
    while (true) {
        Entry& e = buffer_[clock_hand_];
        if (!e.occupied) {
            return clock_hand_;
        }
        if (e.ref_bit.load(std::memory_order_relaxed)) {
            e.ref_bit.store(false, std::memory_order_relaxed);
        } else {
            hashmap_.erase(e.key);
            e.occupied = false;
            return clock_hand_;
        }
        clock_hand_ = (clock_hand_ + 1) % capacity_;
    }
}

template <typename K, typename V>
std::optional<V> ClockCache<K, V>::get(const K& key) {
    if (!bloom_.maybe_contains(key)) {
        return std::nullopt;
    }
    std::shared_lock<std::shared_mutex> lock(mtx_);
    auto it = hashmap_.find(key);
    if (it == hashmap_.end()) {
        return std::nullopt;
    }
    buffer_[it->second].ref_bit.store(true, std::memory_order_relaxed);
    return buffer_[it->second].value;
}

template <typename K, typename V>
void ClockCache<K, V>::put(const K& key, const V& value) {
    std::unique_lock<std::shared_mutex> lock(mtx_);
    if (capacity_ == 0) return;

    auto it = hashmap_.find(key);
    if (it != hashmap_.end()) {
        buffer_[it->second].value = value;
        buffer_[it->second].ref_bit.store(true, std::memory_order_relaxed);
        bloom_.add(key);
        return;
    }

    if (static_cast<int>(hashmap_.size()) >= capacity_) {
        evict();
    }

    // Find the slot: clock_hand_ points at the free slot after evict(),
    // or we need to find a free slot if cache wasn't full.
    size_t slot;
    if (!buffer_[clock_hand_].occupied) {
        slot = clock_hand_;
    } else {
        // Scan for a free slot (only happens when not full, shouldn't hit this after evict)
        slot = evict();
    }

    buffer_[slot].key = key;
    buffer_[slot].value = value;
    buffer_[slot].ref_bit.store(true, std::memory_order_relaxed);
    buffer_[slot].occupied = true;
    hashmap_[key] = slot;
    bloom_.add(key);
    clock_hand_ = (slot + 1) % capacity_;
}

template <typename K, typename V>
bool ClockCache<K, V>::contains(const K& key) {
    std::shared_lock<std::shared_mutex> lock(mtx_);
    return hashmap_.count(key) > 0;
}

template <typename K, typename V>
void ClockCache<K, V>::clear() {
    std::unique_lock<std::shared_mutex> lock(mtx_);
    for (auto& entry : buffer_) {
        entry.occupied = false;
        entry.ref_bit.store(false, std::memory_order_relaxed);
    }
    hashmap_.clear();
    bloom_.clear();
    clock_hand_ = 0;
}

template <typename K, typename V>
int ClockCache<K, V>::getSize() {
    std::shared_lock<std::shared_mutex> lock(mtx_);
    return static_cast<int>(hashmap_.size());
}

template <typename K, typename V>
int ClockCache<K, V>::getCapacity() {
    std::shared_lock<std::shared_mutex> lock(mtx_);
    return capacity_;
}

template <typename K, typename V>
bool ClockCache<K, V>::empty() {
    std::shared_lock<std::shared_mutex> lock(mtx_);
    return hashmap_.empty();
}

template <typename K, typename V>
bool ClockCache<K, V>::full() {
    std::shared_lock<std::shared_mutex> lock(mtx_);
    return static_cast<int>(hashmap_.size()) == capacity_;
}

} // namespace clock_cache
