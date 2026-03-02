#include "lru_cache/lru_cache.hpp"
#include "tsafe_lru_cache/tsafe_cache.hpp"
#include "tsafe_lru_cache/shard.hpp"
#include "tsafe_lru_cache/clock_cache.hpp"
#include "tsafe_lru_cache/sharded_clock.hpp"
#include <atomic>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <random>
#include <string>
#include <thread>
#include <vector>

using Clock = std::chrono::high_resolution_clock;

// ========== Single-threaded baseline ==========

void bench_basic_lru(int cache_size, int num_ops) {
    lru_cache::LRUCache<int, std::string> cache(cache_size);

    std::mt19937 gen(42);
    std::uniform_int_distribution<> key_dis(0, cache_size * 2);

    // Pre-fill
    for (int i = 0; i < cache_size; ++i)
        cache.put(i, "v" + std::to_string(i));

    auto start = Clock::now();
    for (int i = 0; i < num_ops; ++i) {
        int key = key_dis(gen);
        if (i % 10 < 7)
            cache.get(key);
        else
            cache.put(key, "v" + std::to_string(key));
    }
    auto end = Clock::now();

    auto us = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    double ops_per_sec = (double)num_ops / us * 1e6;
    std::cout << "  BasicLRU (single-thread)  : "
              << std::fixed << std::setprecision(0)
              << ops_per_sec << " ops/s  (" << us << " us)\n";
}

// ========== Multi-threaded helper ==========

template <typename CacheT>
double bench_concurrent(const std::string& label, CacheT& cache, int cache_size,
                        int num_threads, int ops_per_thread, int read_pct = 95) {
    // Pre-fill
    for (int i = 0; i < cache_size; ++i)
        cache.put(i, "v" + std::to_string(i));

    std::atomic<bool> go{false};
    std::vector<std::thread> threads;

    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&, t]() {
            std::mt19937 gen(t + 1);
            std::uniform_int_distribution<> key_dis(0, cache_size * 2);
            std::uniform_int_distribution<> op_dis(0, 99);
            while (!go.load(std::memory_order_acquire)) {}  // spin until start

            for (int i = 0; i < ops_per_thread; ++i) {
                int key = key_dis(gen);
                if (op_dis(gen) < read_pct)
                    cache.get(key);
                else
                    cache.put(key, "v" + std::to_string(key));
            }
        });
    }

    auto start = Clock::now();
    go.store(true, std::memory_order_release);
    for (auto& t : threads) t.join();
    auto end = Clock::now();

    int total_ops = num_threads * ops_per_thread;
    auto us = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    double ops_per_sec = (double)total_ops / us * 1e6;
    std::cout << "  " << std::left << std::setw(30) << label << ": "
              << std::fixed << std::setprecision(0)
              << ops_per_sec << " ops/s\n";
    return ops_per_sec;
}

int main() {
    const int CACHE_SIZE = 10000;
    const int OPS = 1000000;

    std::cout << "=== Cache Comparison Benchmark ===\n";
    std::cout << "Cache size: " << CACHE_SIZE
              << " | Total ops: " << OPS << "\n\n";
    std::cout << "A = TSafeCache          single lock, unique_lock for get()\n";
    std::cout << "B = Sharded(TSafe)      16 shards of TSafeCache  (sharding only)\n";
    std::cout << "C = Sharded(Clock+Bloom) 16 shards of ClockCache (sharding + shared_lock + bloom)\n\n";
    std::cout << "Improvement from sharding alone         = (B - A) / A\n";
    std::cout << "Improvement from sharding + clock + bloom = (C - A) / A\n\n";

    for (int read_pct : {70, 95, 100}) {
        std::cout << "==================== " << read_pct << "% reads / "
                  << (100 - read_pct) << "% writes ====================\n\n";

        std::cout << std::left << std::setw(10) << "Threads"
                  << std::setw(16) << "A (ops/s)"
                  << std::setw(16) << "B (ops/s)"
                  << std::setw(16) << "C (ops/s)"
                  << std::setw(14) << "B vs A"
                  << std::setw(14) << "C vs A" << "\n";
        std::cout << std::string(86, '-') << "\n";

        for (int num_threads : {1, 2, 4, 8, 16}) {
            int per_thread = OPS / num_threads;

            double a, b, c;
            // Suppress per-line output by redirecting to /dev/null via a flag
            // Actually, just collect the numbers silently
            {
                tsafe_cache::TSafeCache<int, std::string> tc(CACHE_SIZE);
                // Pre-fill
                for (int i = 0; i < CACHE_SIZE; ++i)
                    tc.put(i, "v" + std::to_string(i));

                std::atomic<bool> go{false};
                std::vector<std::thread> threads;
                for (int t = 0; t < num_threads; ++t) {
                    threads.emplace_back([&, t]() {
                        std::mt19937 gen(t + 1);
                        std::uniform_int_distribution<> key_dis(0, CACHE_SIZE * 2);
                        std::uniform_int_distribution<> op_dis(0, 99);
                        while (!go.load(std::memory_order_acquire)) {}
                        for (int i = 0; i < per_thread; ++i) {
                            int key = key_dis(gen);
                            if (op_dis(gen) < read_pct) tc.get(key);
                            else tc.put(key, "v" + std::to_string(key));
                        }
                    });
                }
                auto start = Clock::now();
                go.store(true, std::memory_order_release);
                for (auto& t : threads) t.join();
                auto us = std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - start).count();
                a = (double)(num_threads * per_thread) / us * 1e6;
            }
            {
                sharded_cache::ShardedLRUCache<int, std::string> sc(CACHE_SIZE);
                for (int i = 0; i < CACHE_SIZE; ++i)
                    sc.put(i, "v" + std::to_string(i));

                std::atomic<bool> go{false};
                std::vector<std::thread> threads;
                for (int t = 0; t < num_threads; ++t) {
                    threads.emplace_back([&, t]() {
                        std::mt19937 gen(t + 1);
                        std::uniform_int_distribution<> key_dis(0, CACHE_SIZE * 2);
                        std::uniform_int_distribution<> op_dis(0, 99);
                        while (!go.load(std::memory_order_acquire)) {}
                        for (int i = 0; i < per_thread; ++i) {
                            int key = key_dis(gen);
                            if (op_dis(gen) < read_pct) sc.get(key);
                            else sc.put(key, "v" + std::to_string(key));
                        }
                    });
                }
                auto start = Clock::now();
                go.store(true, std::memory_order_release);
                for (auto& t : threads) t.join();
                auto us = std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - start).count();
                b = (double)(num_threads * per_thread) / us * 1e6;
            }
            {
                sharded_clock::ShardedClockCache<int, std::string> scc(CACHE_SIZE);
                for (int i = 0; i < CACHE_SIZE; ++i)
                    scc.put(i, "v" + std::to_string(i));

                std::atomic<bool> go{false};
                std::vector<std::thread> threads;
                for (int t = 0; t < num_threads; ++t) {
                    threads.emplace_back([&, t]() {
                        std::mt19937 gen(t + 1);
                        std::uniform_int_distribution<> key_dis(0, CACHE_SIZE * 2);
                        std::uniform_int_distribution<> op_dis(0, 99);
                        while (!go.load(std::memory_order_acquire)) {}
                        for (int i = 0; i < per_thread; ++i) {
                            int key = key_dis(gen);
                            if (op_dis(gen) < read_pct) scc.get(key);
                            else scc.put(key, "v" + std::to_string(key));
                        }
                    });
                }
                auto start = Clock::now();
                go.store(true, std::memory_order_release);
                for (auto& t : threads) t.join();
                auto us = std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - start).count();
                c = (double)(num_threads * per_thread) / us * 1e6;
            }

            double b_vs_a = (b - a) / a * 100.0;
            double c_vs_a = (c - a) / a * 100.0;

            std::cout << std::left << std::setw(10) << num_threads
                      << std::fixed << std::setprecision(0)
                      << std::setw(16) << a
                      << std::setw(16) << b
                      << std::setw(16) << c
                      << std::setprecision(0) << std::showpos
                      << std::setw(14) << (std::string(b_vs_a >= 0 ? "+" : "") + std::to_string((int)b_vs_a) + "%")
                      << std::setw(14) << (std::string(c_vs_a >= 0 ? "+" : "") + std::to_string((int)c_vs_a) + "%")
                      << std::noshowpos << "\n";
        }
        std::cout << "\n";
    }

    std::cout << "Done.\n";
    return 0;
}
