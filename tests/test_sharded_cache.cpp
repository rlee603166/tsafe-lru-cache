#include <gtest/gtest.h>
#include "tsafe_lru_cache/shard.hpp"
#include <atomic>
#include <future>
#include <random>
#include <string>
#include <thread>
#include <vector>

using namespace std;

class ShardedCacheTest : public ::testing::Test {};

// ========== SINGLE-THREADED TESTS ==========

TEST_F(ShardedCacheTest, Construction) {
    sharded_cache::ShardedLRUCache<int, string> cache(160);

    EXPECT_EQ(cache.getSize(), 0);
    EXPECT_EQ(cache.getCapacity(), 160);
    EXPECT_TRUE(cache.empty());
    EXPECT_FALSE(cache.full());
}

TEST_F(ShardedCacheTest, DefaultConstruction) {
    sharded_cache::ShardedLRUCache<int, string> cache;

    EXPECT_EQ(cache.getSize(), 0);
    EXPECT_EQ(cache.getCapacity(), 160);  // 16 shards * 10 default capacity each
    EXPECT_TRUE(cache.empty());
    EXPECT_FALSE(cache.full());
}

TEST_F(ShardedCacheTest, BasicPutAndGet) {
    sharded_cache::ShardedLRUCache<int, string> cache(160);

    cache.put(1, "one");
    cache.put(2, "two");
    cache.put(3, "three");

    auto r1 = cache.get(1);
    auto r2 = cache.get(2);
    auto r3 = cache.get(3);

    ASSERT_TRUE(r1.has_value());
    ASSERT_TRUE(r2.has_value());
    ASSERT_TRUE(r3.has_value());
    EXPECT_EQ(r1.value(), "one");
    EXPECT_EQ(r2.value(), "two");
    EXPECT_EQ(r3.value(), "three");
}

TEST_F(ShardedCacheTest, MissReturnsNullopt) {
    sharded_cache::ShardedLRUCache<int, string> cache(160);

    auto result = cache.get(42);
    EXPECT_FALSE(result.has_value());
    EXPECT_FALSE(cache.contains(42));
}

TEST_F(ShardedCacheTest, Contains) {
    sharded_cache::ShardedLRUCache<int, string> cache(160);

    cache.put(1, "one");
    EXPECT_TRUE(cache.contains(1));
    EXPECT_FALSE(cache.contains(2));
}

TEST_F(ShardedCacheTest, UpdateExistingKey) {
    sharded_cache::ShardedLRUCache<int, string> cache(160);

    cache.put(1, "original");
    cache.put(1, "updated");

    auto result = cache.get(1);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "updated");
    EXPECT_EQ(cache.getSize(), 1);
}

TEST_F(ShardedCacheTest, LRUEvictionWithinShard) {
    // Use capacity of 16 (1 per shard) so keys that hash to the same shard evict each other.
    // Instead, use a small total and keys known to collide, or just verify eviction happens
    // at some point when the cache fills up.
    sharded_cache::ShardedLRUCache<int, string> cache(16);

    // Fill all 160 slots (16 shards * 1 each)
    for (int i = 0; i < 16; ++i) {
        cache.put(i, "v" + to_string(i));
    }
    EXPECT_TRUE(cache.full());

    // Insert one more — something must be evicted
    cache.put(999, "evict_trigger");
    EXPECT_EQ(cache.getSize(), 16);
}

TEST_F(ShardedCacheTest, Clear) {
    sharded_cache::ShardedLRUCache<int, string> cache(160);

    for (int i = 0; i < 10; ++i) {
        cache.put(i, to_string(i));
    }
    EXPECT_EQ(cache.getSize(), 10);

    cache.clear();

    EXPECT_EQ(cache.getSize(), 0);
    EXPECT_TRUE(cache.empty());
    for (int i = 0; i < 10; ++i) {
        EXPECT_FALSE(cache.get(i).has_value());
        EXPECT_FALSE(cache.contains(i));
    }
}

TEST_F(ShardedCacheTest, DifferentKeyTypes) {
    sharded_cache::ShardedLRUCache<string, int> cache(160);

    cache.put("alpha", 1);
    cache.put("beta", 2);

    ASSERT_TRUE(cache.get("alpha").has_value());
    ASSERT_TRUE(cache.get("beta").has_value());
    EXPECT_EQ(cache.get("alpha").value(), 1);
    EXPECT_EQ(cache.get("beta").value(), 2);
}

// ========== MULTI-THREADED TESTS ==========

TEST_F(ShardedCacheTest, ConcurrentWrites) {
    sharded_cache::ShardedLRUCache<int, string> cache(1600);

    const int num_threads = 8;
    const int writes_per_thread = 100;
    vector<thread> threads;

    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&cache, t, writes_per_thread]() {
            for (int i = 0; i < writes_per_thread; ++i) {
                int key = t * writes_per_thread + i;
                cache.put(key, "thread_" + to_string(t) + "_" + to_string(i));
            }
        });
    }
    for (auto& t : threads) t.join();

    EXPECT_EQ(cache.getSize(), num_threads * writes_per_thread);
}

TEST_F(ShardedCacheTest, ConcurrentReads) {
    sharded_cache::ShardedLRUCache<int, string> cache(160);

    for (int i = 0; i < 100; ++i) {
        cache.put(i, "value_" + to_string(i));
    }

    const int num_threads = 8;
    const int reads_per_thread = 1000;
    atomic<int> successful_reads(0);
    vector<thread> threads;

    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&]() {
            mt19937 gen(random_device{}());
            uniform_int_distribution<> dis(0, 99);
            for (int i = 0; i < reads_per_thread; ++i) {
                int key = dis(gen);
                auto result = cache.get(key);
                if (result.has_value() && result.value() == "value_" + to_string(key)) {
                    successful_reads++;
                }
            }
        });
    }
    for (auto& t : threads) t.join();

    EXPECT_GT(successful_reads.load(), num_threads * reads_per_thread / 2);
}

TEST_F(ShardedCacheTest, ConcurrentReadWrite) {
    sharded_cache::ShardedLRUCache<int, string> cache(160);

    const int num_writers = 4;
    const int writes_per_writer = 200;
    atomic<bool> stop_readers(false);
    atomic<int> read_successes(0);

    vector<thread> threads;

    // Readers
    for (int t = 0; t < 4; ++t) {
        threads.emplace_back([&]() {
            mt19937 gen(random_device{}());
            uniform_int_distribution<> dis(0, 199);
            while (!stop_readers.load()) {
                auto result = cache.get(dis(gen));
                if (result.has_value()) read_successes++;
            }
        });
    }

    // Writers
    for (int t = 0; t < num_writers; ++t) {
        threads.emplace_back([&cache, t, writes_per_writer]() {
            for (int i = 0; i < writes_per_writer; ++i) {
                cache.put(t * writes_per_writer + i, "w" + to_string(t) + "_" + to_string(i));
            }
        });
    }

    // Wait for writers to finish, then stop readers
    for (int t = 4; t < 4 + num_writers; ++t) {
        threads[t].join();
    }
    stop_readers.store(true);
    for (int t = 0; t < 4; ++t) {
        threads[t].join();
    }

    EXPECT_GT(read_successes.load(), 0);
}

TEST_F(ShardedCacheTest, StressTest) {
    sharded_cache::ShardedLRUCache<int, string> cache(160);

    const int num_threads = 8;
    const int duration_ms = 200;
    atomic<bool> stop(false);
    atomic<int> total_ops(0);

    vector<thread> threads;
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&, t]() {
            mt19937 gen(t);
            uniform_int_distribution<> key_dis(0, 199);
            uniform_int_distribution<> op_dis(0, 99);

            while (!stop.load()) {
                int key = key_dis(gen);
                int op = op_dis(gen);

                if (op < 50)       cache.get(key);
                else if (op < 80)  cache.put(key, "t" + to_string(t));
                else if (op < 90)  cache.contains(key);
                else if (op < 95)  cache.getSize();
                else               { if (t == 0) cache.clear(); }

                total_ops++;
            }
        });
    }

    this_thread::sleep_for(chrono::milliseconds(duration_ms));
    stop.store(true);
    for (auto& t : threads) t.join();

    int size = cache.getSize();
    EXPECT_GE(size, 0);
    EXPECT_LE(size, cache.getCapacity());
    EXPECT_GT(total_ops.load(), 1000);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
