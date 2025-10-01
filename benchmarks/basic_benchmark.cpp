#include <benchmark/benchmark.h>
#include "tsafe_lru_cache/basic_lru_cache.hpp"
#include <random>
#include <string>

// Benchmark basic LRU cache operations
static void BM_BasicLRU_Put(benchmark::State& state) {
    const size_t cache_size = state.range(0);
    basic_cache::LRUCache<int, std::string> cache(cache_size);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, cache_size * 2);

    for (auto _ : state) {
        int key = dis(gen);
        cache.put(key, "value_" + std::to_string(key));
        benchmark::DoNotOptimize(cache);
    }

    state.SetComplexityN(cache_size);
}

static void BM_BasicLRU_Get(benchmark::State& state) {
    const size_t cache_size = state.range(0);
    basic_cache::LRUCache<int, std::string> cache(cache_size);

    // Pre-fill cache
    for (int i = 1; i <= static_cast<int>(cache_size); ++i) {
        cache.put(i, "value_" + std::to_string(i));
    }

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, cache_size);

    for (auto _ : state) {
        int key = dis(gen);
        auto result = cache.get(key);
        benchmark::DoNotOptimize(result);
    }

    state.SetComplexityN(cache_size);
}

static void BM_BasicLRU_Mixed(benchmark::State& state) {
    const size_t cache_size = state.range(0);
    basic_cache::LRUCache<int, std::string> cache(cache_size);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> key_dis(1, cache_size * 2);
    std::uniform_real_distribution<> op_dis(0.0, 1.0);

    for (auto _ : state) {
        int key = key_dis(gen);
        if (op_dis(gen) < 0.7) {  // 70% reads, 30% writes
            auto result = cache.get(key);
            benchmark::DoNotOptimize(result);
        } else {
            cache.put(key, "value_" + std::to_string(key));
            benchmark::DoNotOptimize(cache);
        }
    }

    state.SetComplexityN(cache_size);
}

// Register benchmarks with different cache sizes
BENCHMARK(BM_BasicLRU_Put)->Range(8, 8192)->Complexity();
BENCHMARK(BM_BasicLRU_Get)->Range(8, 8192)->Complexity();
BENCHMARK(BM_BasicLRU_Mixed)->Range(8, 8192)->Complexity();

BENCHMARK_MAIN();