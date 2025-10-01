# =====================================================
# Examples
# =====================================================

# Basic Demo (Phase 1)
add_executable(basic_demo basic_demo.cpp)
target_link_libraries(basic_demo PRIVATE TSafeLRUCache::tsafe_lru_cache)

# Future examples will be added here as phases progress
# add_executable(threadsafe_demo threadsafe_demo.cpp)
# add_executable(sharded_demo sharded_demo.cpp)
# add_executable(grpc_client_demo grpc_client_demo.cpp)