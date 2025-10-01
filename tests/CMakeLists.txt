# tests/CMakeLists.txt

# Create test executables
add_executable(test_basic_lru tests/test_basic_lru_cache.cpp)
add_executable(test_tsafe_cache tests/test_tsafe_cache.cpp)

# Link against our header-only library
target_link_libraries(test_basic_lru 
    TSafeLRUCache::tsafe_lru_cache
    ${GTEST_LIBRARIES}
)

target_link_libraries(test_tsafe_cache 
    TSafeLRUCache::tsafe_lru_cache
    ${GTEST_LIBRARIES}
)

# Add tests to CTest
add_test(NAME BasicLRUTests COMMAND test_basic_lru)
add_test(NAME TSafeCacheTests COMMAND test_tsafe_cache)

# Set test properties
set_tests_properties(BasicLRUTests PROPERTIES TIMEOUT 30)
set_tests_properties(TSafeCacheTests PROPERTIES TIMEOUT 60)  # Longer timeout for multi-threaded tests
