# =====================================================
# Benchmarks (Phase 4)
# =====================================================

if(NOT BENCHMARKING_ENABLED)
    message(WARNING "Benchmarking disabled - Google Benchmark not found")
    return()
endif()

# Basic LRU Cache Benchmark
add_executable(basic_benchmark basic_benchmark.cpp)
target_link_libraries(basic_benchmark PRIVATE
    TSafeLRUCache::tsafe_lru_cache
    benchmark::benchmark
)

# Future benchmarks will be added here
# add_executable(threadsafe_benchmark threadsafe_benchmark.cpp)
# add_executable(sharded_benchmark sharded_benchmark.cpp)
# add_executable(performance_comparison performance_comparison.cpp)