#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>

namespace bloom {

template <typename K>
class BloomFilter {
private:
    size_t num_bits_;
    std::vector<std::atomic<uint8_t>> bits_;

    // Two hash functions derived from std::hash via murmur-style mixing
    size_t hash1(const K& key) const {
        size_t h = std::hash<K>{}(key);
        h ^= h >> 33;
        h *= 0xff51afd7ed558ccdULL;
        h ^= h >> 33;
        return h % num_bits_;
    }

    size_t hash2(const K& key) const {
        size_t h = std::hash<K>{}(key);
        h ^= h >> 31;
        h *= 0xc4ceb9fe1a85ec53ULL;
        h ^= h >> 31;
        return h % num_bits_;
    }

public:
    explicit BloomFilter(size_t capacity)
        : num_bits_(capacity * 10 > 0 ? capacity * 10 : 64),
          bits_(capacity * 10 > 0 ? capacity * 10 / 8 + 1 : 8) {
        for (auto& b : bits_) b.store(0, std::memory_order_relaxed);
    }

    void add(const K& key) {
        size_t b1 = hash1(key);
        size_t b2 = hash2(key);

        size_t byte1 = b1 / 8, bit1 = b1 % 8;
        size_t byte2 = b2 / 8, bit2 = b2 % 8;

        bits_[byte1].fetch_or(uint8_t(1) << bit1, std::memory_order_relaxed);
        bits_[byte2].fetch_or(uint8_t(1) << bit2, std::memory_order_relaxed);
    }

    bool maybe_contains(const K& key) const {
        size_t b1 = hash1(key);
        size_t b2 = hash2(key);

        size_t byte1 = b1 / 8, bit1 = b1 % 8;
        size_t byte2 = b2 / 8, bit2 = b2 % 8;

        uint8_t v1 = bits_[byte1].load(std::memory_order_relaxed);
        uint8_t v2 = bits_[byte2].load(std::memory_order_relaxed);

        return (v1 & (uint8_t(1) << bit1)) && (v2 & (uint8_t(1) << bit2));
    }

    void clear() {
        for (auto& b : bits_) b.store(0, std::memory_order_relaxed);
    }
};

} // namespace bloom
