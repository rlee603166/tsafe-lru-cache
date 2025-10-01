#include <gtest/gtest.h>
#include "tsafe_lru_cache/tsafe_cache.hpp"
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <atomic>
#include <random>
#include <future>

using namespace std;

class TSafeCacheTest : public ::testing::Test {};

// ========== SINGLE-THREADED TESTS (Basic Functionality) ==========

TEST_F(TSafeCacheTest, Construction) {
    tsafe_cache::TSafeCache<int, string> cache(3);

    EXPECT_EQ(cache.getSize(), 0);
    EXPECT_EQ(cache.getCapacity(), 3);
    EXPECT_TRUE(cache.empty());
    EXPECT_FALSE(cache.full());
}

TEST_F(TSafeCacheTest, DefaultConstruction) {
    tsafe_cache::TSafeCache<int, string> cache;  // Should use default capacity of 10

    EXPECT_EQ(cache.getSize(), 0);
    EXPECT_EQ(cache.getCapacity(), 10);
    EXPECT_TRUE(cache.empty());
    EXPECT_FALSE(cache.full());
}

TEST_F(TSafeCacheTest, ZeroCapacity) {
    tsafe_cache::TSafeCache<int, string> cache(0);    

    EXPECT_EQ(cache.getSize(), 0);
    EXPECT_EQ(cache.getCapacity(), 0);
    EXPECT_TRUE(cache.empty());
    EXPECT_TRUE(cache.full());  // Zero capacity cache is always full

    cache.put(1, "100");
    EXPECT_EQ(cache.getSize(), 0);

    auto result = cache.get(1);
    EXPECT_FALSE(result.has_value());
    EXPECT_FALSE(cache.contains(1));
}

TEST_F(TSafeCacheTest, SingleElement) {
    tsafe_cache::TSafeCache<int, string> cache(1);
    
    // Insert single element
    cache.put(1, "one");
    EXPECT_EQ(cache.getSize(), 1);
    EXPECT_FALSE(cache.empty());
    EXPECT_TRUE(cache.full());
    EXPECT_TRUE(cache.contains(1));
    
    // Retrieve it
    auto result = cache.get(1);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "one");
    
    // Insert another element (should evict the first)
    cache.put(2, "two");
    EXPECT_EQ(cache.getSize(), 1);
    EXPECT_TRUE(cache.full());
    
    // First element should be gone
    auto result1 = cache.get(1);
    EXPECT_FALSE(result1.has_value());
    EXPECT_FALSE(cache.contains(1));
    
    // Second element should be present
    auto result2 = cache.get(2);
    ASSERT_TRUE(result2.has_value());
    EXPECT_EQ(result2.value(), "two");
    EXPECT_TRUE(cache.contains(2));
}

TEST_F(TSafeCacheTest, BasicOperations) {
    tsafe_cache::TSafeCache<int, string> cache(3);
    
    // Test empty cache get
    auto result = cache.get(1);
    EXPECT_FALSE(result.has_value());
    EXPECT_FALSE(cache.contains(1));
    
    // Insert elements
    cache.put(1, "one");
    cache.put(2, "two");
    cache.put(3, "three");
    
    EXPECT_EQ(cache.getSize(), 3);
    EXPECT_TRUE(cache.full());
    
    // Test successful gets and contains
    auto r1 = cache.get(1);
    auto r2 = cache.get(2);
    auto r3 = cache.get(3);
    
    ASSERT_TRUE(r1.has_value());
    ASSERT_TRUE(r2.has_value());
    ASSERT_TRUE(r3.has_value());
    
    EXPECT_EQ(r1.value(), "one");
    EXPECT_EQ(r2.value(), "two");
    EXPECT_EQ(r3.value(), "three");
    
    EXPECT_TRUE(cache.contains(1));
    EXPECT_TRUE(cache.contains(2));
    EXPECT_TRUE(cache.contains(3));
}

TEST_F(TSafeCacheTest, LRUEviction) {
    tsafe_cache::TSafeCache<int, string> cache(3);
    
    // Fill cache to capacity
    cache.put(1, "one");
    cache.put(2, "two");
    cache.put(3, "three");
    
    // Access element 1 to make it recently used
    cache.get(1);
    
    // Insert element 4 (should evict element 2, the least recently used)
    cache.put(4, "four");
    
    EXPECT_EQ(cache.getSize(), 3);
    
    // Element 2 should be evicted
    auto r2 = cache.get(2);
    EXPECT_FALSE(r2.has_value());
    EXPECT_FALSE(cache.contains(2));
    
    // Elements 1, 3, 4 should still be present
    auto r1 = cache.get(1);
    auto r3 = cache.get(3);
    auto r4 = cache.get(4);
    
    ASSERT_TRUE(r1.has_value());
    ASSERT_TRUE(r3.has_value());
    ASSERT_TRUE(r4.has_value());
    
    EXPECT_EQ(r1.value(), "one");
    EXPECT_EQ(r3.value(), "three");
    EXPECT_EQ(r4.value(), "four");
}

TEST_F(TSafeCacheTest, UpdateExistingKey) {
    tsafe_cache::TSafeCache<int, string> cache(3);
    
    cache.put(1, "original");
    cache.put(2, "two");
    cache.put(3, "three");
    
    // Update existing key
    cache.put(1, "updated");
    
    // Size should remain the same
    EXPECT_EQ(cache.getSize(), 3);
    
    // Value should be updated
    auto result = cache.get(1);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "updated");
}

TEST_F(TSafeCacheTest, Clear) {
    tsafe_cache::TSafeCache<int, string> cache(3);
    
    cache.put(1, "one");
    cache.put(2, "two");
    cache.put(3, "three");
    
    EXPECT_EQ(cache.getSize(), 3);
    EXPECT_FALSE(cache.empty());
    EXPECT_TRUE(cache.full());
    
    cache.clear();
    
    EXPECT_EQ(cache.getSize(), 0);
    EXPECT_TRUE(cache.empty());
    EXPECT_FALSE(cache.full());
    
    // All elements should be gone
    EXPECT_FALSE(cache.get(1).has_value());
    EXPECT_FALSE(cache.get(2).has_value());
    EXPECT_FALSE(cache.get(3).has_value());
    EXPECT_FALSE(cache.contains(1));
    EXPECT_FALSE(cache.contains(2));
    EXPECT_FALSE(cache.contains(3));
    
    // Should be able to use cache normally after clear
    cache.put(4, "four");
    auto r4 = cache.get(4);
    ASSERT_TRUE(r4.has_value());
    EXPECT_EQ(r4.value(), "four");
}

TEST_F(TSafeCacheTest, DifferentTypes) {
    tsafe_cache::TSafeCache<string, int> cache(2);
    
    cache.put("hello", 42);
    cache.put("world", 84);
    
    auto r1 = cache.get("hello");
    auto r2 = cache.get("world");
    
    ASSERT_TRUE(r1.has_value());
    ASSERT_TRUE(r2.has_value());
    EXPECT_EQ(r1.value(), 42);
    EXPECT_EQ(r2.value(), 84);
    
    EXPECT_TRUE(cache.contains("hello"));
    EXPECT_TRUE(cache.contains("world"));
    
    // Test eviction with string keys
    cache.put("foo", 100);
    
    // "hello" should be evicted
    auto r3 = cache.get("hello");
    EXPECT_FALSE(r3.has_value());
    EXPECT_FALSE(cache.contains("hello"));
    
    auto r4 = cache.get("foo");
    ASSERT_TRUE(r4.has_value());
    EXPECT_EQ(r4.value(), 100);
}

// ========== MULTI-THREADED TESTS (Thread Safety) ==========

TEST_F(TSafeCacheTest, ConcurrentReads) {
    tsafe_cache::TSafeCache<int, string> cache(10);
    
    // Pre-populate cache
    for (int i = 0; i < 10; ++i) {
        cache.put(i, "value_" + to_string(i));
    }
    
    const int num_threads = 8;
    const int reads_per_thread = 1000;
    vector<thread> threads;
    atomic<int> successful_reads(0);
    
    // Launch multiple reader threads
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&cache, &successful_reads, reads_per_thread]() {
            random_device rd;
            mt19937 gen(rd());
            uniform_int_distribution<> dis(0, 9);
            
            for (int i = 0; i < reads_per_thread; ++i) {
                int key = dis(gen);
                auto result = cache.get(key);
                if (result.has_value() && result.value() == "value_" + to_string(key)) {
                    successful_reads++;
                }
                
                // Also test contains
                bool contains = cache.contains(key);
                if (contains && result.has_value()) {
                    // Both should agree
                }
            }
        });
    }
    
    // Wait for all threads
    for (auto& t : threads) {
        t.join();
    }
    
    // Should have many successful reads (not necessarily all due to potential evictions)
    EXPECT_GT(successful_reads.load(), num_threads * reads_per_thread / 2);
}

TEST_F(TSafeCacheTest, ConcurrentWrites) {
    tsafe_cache::TSafeCache<int, string> cache(100);
    
    const int num_threads = 4;
    const int writes_per_thread = 250;
    vector<thread> threads;
    
    // Launch multiple writer threads
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&cache, t, writes_per_thread]() {
            for (int i = 0; i < writes_per_thread; ++i) {
                int key = t * writes_per_thread + i;
                cache.put(key, "thread_" + to_string(t) + "_value_" + to_string(i));
            }
        });
    }
    
    // Wait for all threads
    for (auto& t : threads) {
        t.join();
    }
    
    // Cache should be at capacity and contain the most recent entries
    EXPECT_EQ(cache.getSize(), 100);
    EXPECT_TRUE(cache.full());
}

TEST_F(TSafeCacheTest, ConcurrentReadWrite) {
    tsafe_cache::TSafeCache<int, string> cache(50);
    
    const int num_reader_threads = 4;
    const int num_writer_threads = 2;
    const int operations_per_thread = 500;
    
    atomic<bool> stop_flag(false);
    atomic<int> read_successes(0);
    atomic<int> write_count(0);
    
    vector<thread> threads;
    
    // Launch reader threads
    for (int t = 0; t < num_reader_threads; ++t) {
        threads.emplace_back([&]() {
            random_device rd;
            mt19937 gen(rd());
            uniform_int_distribution<> dis(0, 99);
            
            while (!stop_flag.load()) {
                int key = dis(gen);
                auto result = cache.get(key);
                if (result.has_value()) {
                    read_successes++;
                }
                this_thread::sleep_for(chrono::microseconds(10));
            }
        });
    }
    
    // Launch writer threads
    for (int t = 0; t < num_writer_threads; ++t) {
        threads.emplace_back([&, t]() {
            for (int i = 0; i < operations_per_thread; ++i) {
                int key = t * operations_per_thread + i;
                cache.put(key, "writer_" + to_string(t) + "_" + to_string(i));
                write_count++;
                this_thread::sleep_for(chrono::microseconds(50));
            }
        });
    }
    
    // Let it run for a bit
    this_thread::sleep_for(chrono::milliseconds(100));
    
    // Stop readers
    stop_flag.store(true);
    
    // Wait for all threads
    for (auto& t : threads) {
        t.join();
    }
    
    EXPECT_EQ(write_count.load(), num_writer_threads * operations_per_thread);
    EXPECT_GT(read_successes.load(), 0);  // Should have some successful reads
}

TEST_F(TSafeCacheTest, ConcurrentClearAndAccess) {
    tsafe_cache::TSafeCache<int, string> cache(10);
    
    // Pre-populate
    for (int i = 0; i < 10; ++i) {
        cache.put(i, "value_" + to_string(i));
    }
    
    atomic<bool> stop_flag(false);
    atomic<int> operations_completed(0);
    
    vector<thread> threads;
    
    // Thread that periodically clears the cache
    threads.emplace_back([&]() {
        for (int i = 0; i < 5; ++i) {
            this_thread::sleep_for(chrono::milliseconds(20));
            cache.clear();
            
            // Repopulate after clear
            for (int j = 0; j < 5; ++j) {
                cache.put(j, "new_value_" + to_string(j));
            }
        }
        stop_flag.store(true);
    });
    
    // Threads that constantly access the cache
    for (int t = 0; t < 3; ++t) {
        threads.emplace_back([&]() {
            random_device rd;
            mt19937 gen(rd());
            uniform_int_distribution<> dis(0, 9);
            
            while (!stop_flag.load()) {
                int key = dis(gen);
                cache.get(key);
                cache.contains(key);
                operations_completed++;
                this_thread::sleep_for(chrono::microseconds(100));
            }
        });
    }
    
    // Wait for all threads
    for (auto& t : threads) {
        t.join();
    }
    
    // Should have completed many operations without crashing
    EXPECT_GT(operations_completed.load(), 100);
}

TEST_F(TSafeCacheTest, StressTestAllOperations) {
    tsafe_cache::TSafeCache<int, string> cache(20);
    
    const int num_threads = 6;
    const int duration_ms = 200;
    
    atomic<bool> stop_flag(false);
    atomic<int> total_operations(0);
    
    vector<thread> threads;
    
    // Launch threads that perform random operations
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&, t]() {
            random_device rd;
            mt19937 gen(rd());
            uniform_int_distribution<> key_dis(0, 50);
            uniform_int_distribution<> op_dis(0, 100);
            
            while (!stop_flag.load()) {
                int operation = op_dis(gen);
                int key = key_dis(gen);
                
                if (operation < 40) {
                    // 40% reads
                    cache.get(key);
                } else if (operation < 70) {
                    // 30% writes
                    cache.put(key, "thread_" + to_string(t) + "_" + to_string(key));
                } else if (operation < 85) {
                    // 15% contains
                    cache.contains(key);
                } else if (operation < 90) {
                    // 5% size/capacity checks
                    cache.getSize();
                    cache.getCapacity();
                } else if (operation < 95) {
                    // 5% empty/full checks
                    cache.empty();
                    cache.full();
                } else {
                    // 5% clear (rare but important to test)
                    if (t == 0) {  // Only one thread does clears
                        cache.clear();
                    }
                }
                
                total_operations++;
                this_thread::sleep_for(chrono::microseconds(10));
            }
        });
    }
    
    // Run for specified duration
    this_thread::sleep_for(chrono::milliseconds(duration_ms));
    stop_flag.store(true);
    
    // Wait for all threads
    for (auto& t : threads) {
        t.join();
    }
    
    // Verify final state is consistent
    int final_size = cache.getSize();
    int final_capacity = cache.getCapacity();
    bool final_empty = cache.empty();
    bool final_full = cache.full();
    
    EXPECT_GE(final_size, 0);
    EXPECT_LE(final_size, final_capacity);
    EXPECT_EQ(final_empty, (final_size == 0));
    EXPECT_EQ(final_full, (final_size == final_capacity));
    EXPECT_GT(total_operations.load(), 1000);  // Should have done many operations
}

// Test that demonstrates reader-writer lock benefits
TEST_F(TSafeCacheTest, ReaderWriterConcurrency) {
    tsafe_cache::TSafeCache<int, string> cache(100);
    
    // Pre-populate cache
    for (int i = 0; i < 100; ++i) {
        cache.put(i, "initial_value_" + to_string(i));
    }
    
    const int num_readers = 8;
    const int reads_per_reader = 1000;
    
    auto start_time = chrono::high_resolution_clock::now();
    
    vector<future<int>> reader_futures;
    
    // Launch many concurrent readers
    for (int t = 0; t < num_readers; ++t) {
        reader_futures.push_back(async(launch::async, [&cache, reads_per_reader]() {
            int successful_reads = 0;
            random_device rd;
            mt19937 gen(rd());
            uniform_int_distribution<> dis(0, 99);
            
            for (int i = 0; i < reads_per_reader; ++i) {
                int key = dis(gen);
                auto result = cache.get(key);
                if (result.has_value()) {
                    successful_reads++;
                }
            }
            return successful_reads;
        }));
    }
    
    // Wait for all readers
    int total_successful = 0;
    for (auto& future : reader_futures) {
        total_successful += future.get();
    }
    
    auto end_time = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(end_time - start_time);
    
    // With shared_mutex, readers should be able to run concurrently
    // This test mainly ensures no deadlocks occur with many concurrent readers
    EXPECT_GT(total_successful, num_readers * reads_per_reader * 0.9);  // Most reads should succeed
    EXPECT_LT(duration.count(), 1000);  // Should complete reasonably quickly
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
