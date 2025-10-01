# =====================================================
# Unit Tests
# =====================================================

# Basic LRU Cache Tests (Phase 1)
add_executable(test_basic_lru test_basic_lru_cache.cpp)
target_link_libraries(test_basic_lru PRIVATE
    TSafeLRUCache::tsafe_lru_cache
    ${GTEST_LIBRARIES}
)

# Register with CTest
add_test(NAME BasicLRUTests COMMAND test_basic_lru)

# Future unit tests
# add_executable(test_threadsafe_lru test_threadsafe_lru.cpp)
# add_executable(test_sharded_lru test_sharded_lru.cpp)
