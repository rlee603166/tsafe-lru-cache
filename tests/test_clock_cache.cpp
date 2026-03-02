#include <gtest/gtest.h>
#include "tsafe_lru_cache/clock_cache.hpp"
#include "tsafe_lru_cache/sharded_clock.hpp"
#include <atomic>
#include <random>
#include <string>
#include <thread>
#include <vector>

using namespace std;

// ========== ClockCache SINGLE-THREADED TESTS ==========

class ClockCacheTest : public ::testing::Test {};

TEST_F(ClockCacheTest, Construction) {
    clock_cache::ClockCache<int, string> cache(3);

    EXPECT_EQ(cache.getSize(), 0);
    EXPECT_EQ(cache.getCapacity(), 3);
    EXPECT_TRUE(cache.empty());
    EXPECT_FALSE(cache.full());
}

TEST_F(ClockCacheTest, ZeroCapacity) {
    clock_cache::ClockCache<int, string> cache(0);

    EXPECT_EQ(cache.getSize(), 0);
    EXPECT_EQ(cache.getCapacity(), 0);

    cache.put(1, "one");
    EXPECT_EQ(cache.getSize(), 0);

    auto result = cache.get(1);
    EXPECT_FALSE(result.has_value());
}

TEST_F(ClockCacheTest, BasicPutAndGet) {
    clock_cache::ClockCache<int, string> cache(3);

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

    EXPECT_EQ(cache.getSize(), 3);
    EXPECT_TRUE(cache.full());
}

TEST_F(ClockCacheTest, MissReturnsNullopt) {
    clock_cache::ClockCache<int, string> cache(3);

    auto result = cache.get(42);
    EXPECT_FALSE(result.has_value());
    EXPECT_FALSE(cache.contains(42));
}

TEST_F(ClockCacheTest, Contains) {
    clock_cache::ClockCache<int, string> cache(3);

    cache.put(1, "one");
    EXPECT_TRUE(cache.contains(1));
    EXPECT_FALSE(cache.contains(2));
}

TEST_F(ClockCacheTest, UpdateExistingKey) {
    clock_cache::ClockCache<int, string> cache(3);

    cache.put(1, "original");
    cache.put(1, "updated");

    auto result = cache.get(1);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "updated");
    EXPECT_EQ(cache.getSize(), 1);
}

TEST_F(ClockCacheTest, EvictionHappens) {
    clock_cache::ClockCache<int, string> cache(3);

    cache.put(1, "one");
    cache.put(2, "two");
    cache.put(3, "three");

    // Cache is full, inserting a 4th should evict something
    cache.put(4, "four");
    EXPECT_EQ(cache.getSize(), 3);

    // Key 4 must be present
    auto r4 = cache.get(4);
    ASSERT_TRUE(r4.has_value());
    EXPECT_EQ(r4.value(), "four");

    // At least one of 1,2,3 must have been evicted
    int present = 0;
    if (cache.get(1).has_value()) present++;
    if (cache.get(2).has_value()) present++;
    if (cache.get(3).has_value()) present++;
    EXPECT_EQ(present, 2);
}

TEST_F(ClockCacheTest, SecondChanceEviction) {
    // CLOCK gives a second chance to recently accessed entries.
    // After a first eviction clears ref bits during its sweep, a subsequent
    // get() re-sets the bit, protecting that entry from the next eviction.
    clock_cache::ClockCache<int, string> cache(3);

    cache.put(1, "one");   // slot 0, ref=true
    cache.put(2, "two");   // slot 1, ref=true
    cache.put(3, "three"); // slot 2, ref=true, hand wraps to 0

    // First eviction: hand sweeps 0,1,2 clearing all ref bits,
    // wraps to 0 and evicts key 1 (first with ref=false).
    cache.put(4, "four");  // evicts key 1 from slot 0, hand moves to 1
    EXPECT_FALSE(cache.get(1).has_value());  // evicted
    EXPECT_TRUE(cache.get(4).has_value());   // present in slot 0

    // Now: slot 0 (key 4, ref=true from get), slot 1 (key 2, ref=false),
    //       slot 2 (key 3, ref=false), hand=1
    // Access key 3 to give it a second chance
    cache.get(3);  // sets ref=true on slot 2

    // Second eviction: hand at 1, key 2 ref=false → evict key 2
    cache.put(5, "five");
    EXPECT_FALSE(cache.get(2).has_value());  // evicted
    EXPECT_TRUE(cache.get(3).has_value());   // survived (second chance from get)
    EXPECT_TRUE(cache.get(4).has_value());   // still present
    EXPECT_TRUE(cache.get(5).has_value());   // newly inserted
}

TEST_F(ClockCacheTest, DeterministicSweepOrder) {
    // With no ref bits set, CLOCK evicts in insertion order (round-robin).
    clock_cache::ClockCache<int, int> cache(3);

    cache.put(10, 100);  // slot 0
    cache.put(20, 200);  // slot 1
    cache.put(30, 300);  // slot 2

    // Clear all ref bits by NOT accessing any keys via get().
    // The put() sets ref_bit=true, so we need to sweep through once.
    // Actually, put sets ref_bit. Let's force a known state by doing
    // two rounds of eviction so the hand wraps predictably.

    // Insert 40 — hand at 0, key 10 has ref_bit=true → clear, move to slot 1
    //   key 20 has ref_bit=true → clear, move to slot 2
    //   key 30 has ref_bit=true → clear, move to slot 0
    //   key 10 now has ref_bit=false → evict slot 0
    cache.put(40, 400);
    EXPECT_FALSE(cache.get(10).has_value()); // evicted
    EXPECT_TRUE(cache.get(20).has_value());
    EXPECT_TRUE(cache.get(30).has_value());
    EXPECT_TRUE(cache.get(40).has_value());
}

TEST_F(ClockCacheTest, Clear) {
    clock_cache::ClockCache<int, string> cache(3);

    cache.put(1, "one");
    cache.put(2, "two");
    cache.put(3, "three");

    cache.clear();

    EXPECT_EQ(cache.getSize(), 0);
    EXPECT_TRUE(cache.empty());
    EXPECT_FALSE(cache.get(1).has_value());
    EXPECT_FALSE(cache.get(2).has_value());
    EXPECT_FALSE(cache.get(3).has_value());

    // Can reuse after clear
    cache.put(4, "four");
    auto r4 = cache.get(4);
    ASSERT_TRUE(r4.has_value());
    EXPECT_EQ(r4.value(), "four");
}

TEST_F(ClockCacheTest, DifferentTypes) {
    clock_cache::ClockCache<string, int> cache(2);

    cache.put("hello", 42);
    cache.put("world", 84);

    ASSERT_TRUE(cache.get("hello").has_value());
    ASSERT_TRUE(cache.get("world").has_value());
    EXPECT_EQ(cache.get("hello").value(), 42);
    EXPECT_EQ(cache.get("world").value(), 84);
}

TEST_F(ClockCacheTest, LargeCapacity) {
    const int capacity = 1000;
    clock_cache::ClockCache<int, int> cache(capacity);

    for (int i = 0; i < capacity; ++i) {
        cache.put(i, i * 2);
    }

    EXPECT_EQ(cache.getSize(), capacity);
    EXPECT_TRUE(cache.full());

    for (int i = 0; i < capacity; ++i) {
        auto result = cache.get(i);
        ASSERT_TRUE(result.has_value()) << "Failed to get key: " << i;
        EXPECT_EQ(result.value(), i * 2);
    }
}

// ========== ClockCache MULTI-THREADED TESTS ==========

TEST_F(ClockCacheTest, ConcurrentReads) {
    clock_cache::ClockCache<int, string> cache(100);

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

    // All reads should succeed since we only read (no eviction)
    EXPECT_EQ(successful_reads.load(), num_threads * reads_per_thread);
}

TEST_F(ClockCacheTest, ConcurrentWrites) {
    clock_cache::ClockCache<int, string> cache(1600);

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

TEST_F(ClockCacheTest, ConcurrentReadWrite) {
    clock_cache::ClockCache<int, string> cache(160);

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

    // Wait for writers, then stop readers
    for (int t = 4; t < 4 + num_writers; ++t) {
        threads[t].join();
    }
    stop_readers.store(true);
    for (int t = 0; t < 4; ++t) {
        threads[t].join();
    }

    EXPECT_GT(read_successes.load(), 0);
}

TEST_F(ClockCacheTest, StressTest) {
    clock_cache::ClockCache<int, string> cache(160);

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

// ========== Bloom Filter Tests ==========

TEST_F(ClockCacheTest, BloomNoFalseNegatives) {
    // A Bloom filter must never produce false negatives:
    // if we inserted a key, maybe_contains must return true.
    clock_cache::ClockCache<int, string> cache(1000);

    for (int i = 0; i < 1000; ++i) {
        cache.put(i, "v" + to_string(i));
    }

    for (int i = 0; i < 1000; ++i) {
        auto result = cache.get(i);
        ASSERT_TRUE(result.has_value()) << "Bloom filter false negative for key " << i;
        EXPECT_EQ(result.value(), "v" + to_string(i));
    }
}

TEST_F(ClockCacheTest, BloomShortCircuitsMisses) {
    // Keys never inserted should mostly be short-circuited by the bloom filter.
    // We can't directly observe the short-circuit, but we can verify that
    // get() returns nullopt for keys never put.
    clock_cache::ClockCache<int, string> cache(100);

    for (int i = 0; i < 100; ++i) {
        cache.put(i, "v" + to_string(i));
    }

    int misses = 0;
    for (int i = 1000; i < 2000; ++i) {
        auto result = cache.get(i);
        if (!result.has_value()) misses++;
    }
    // All 1000 lookups for never-inserted keys should miss
    EXPECT_EQ(misses, 1000);
}

TEST_F(ClockCacheTest, BloomSurvivesClear) {
    clock_cache::ClockCache<int, string> cache(100);

    for (int i = 0; i < 100; ++i) {
        cache.put(i, "v" + to_string(i));
    }

    cache.clear();

    // After clear, all gets should miss (bloom was also cleared)
    for (int i = 0; i < 100; ++i) {
        EXPECT_FALSE(cache.get(i).has_value());
    }

    // Re-insert and verify bloom works again
    for (int i = 0; i < 50; ++i) {
        cache.put(i, "new" + to_string(i));
    }
    for (int i = 0; i < 50; ++i) {
        auto result = cache.get(i);
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(result.value(), "new" + to_string(i));
    }
}

TEST_F(ClockCacheTest, BloomConcurrentReadMiss) {
    // Hammer the cache with misses from multiple threads.
    // The bloom filter should safely handle concurrent reads.
    clock_cache::ClockCache<int, string> cache(100);

    for (int i = 0; i < 100; ++i) {
        cache.put(i, "v" + to_string(i));
    }

    const int num_threads = 8;
    const int reads_per_thread = 10000;
    atomic<int> miss_count(0);
    vector<thread> threads;

    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&]() {
            for (int i = 0; i < reads_per_thread; ++i) {
                // Keys 10000+ were never inserted
                auto result = cache.get(10000 + i);
                if (!result.has_value()) miss_count++;
            }
        });
    }
    for (auto& t : threads) t.join();

    EXPECT_EQ(miss_count.load(), num_threads * reads_per_thread);
}

// ========== ShardedClockCache TESTS ==========

class ShardedClockCacheTest : public ::testing::Test {};

TEST_F(ShardedClockCacheTest, Construction) {
    sharded_clock::ShardedClockCache<int, string> cache(160);

    EXPECT_EQ(cache.getSize(), 0);
    EXPECT_EQ(cache.getCapacity(), 160);
    EXPECT_TRUE(cache.empty());
    EXPECT_FALSE(cache.full());
}

TEST_F(ShardedClockCacheTest, BasicPutAndGet) {
    sharded_clock::ShardedClockCache<int, string> cache(160);

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

TEST_F(ShardedClockCacheTest, MissReturnsNullopt) {
    sharded_clock::ShardedClockCache<int, string> cache(160);

    EXPECT_FALSE(cache.get(42).has_value());
    EXPECT_FALSE(cache.contains(42));
}

TEST_F(ShardedClockCacheTest, Clear) {
    sharded_clock::ShardedClockCache<int, string> cache(160);

    for (int i = 0; i < 10; ++i) {
        cache.put(i, to_string(i));
    }
    EXPECT_EQ(cache.getSize(), 10);

    cache.clear();

    EXPECT_EQ(cache.getSize(), 0);
    EXPECT_TRUE(cache.empty());
}

TEST_F(ShardedClockCacheTest, ConcurrentStress) {
    sharded_clock::ShardedClockCache<int, string> cache(160);

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
