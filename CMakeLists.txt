cmake_minimum_required(VERSION 3.10)
project(TSafeLRUCache)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Header + source files
include_directories(include)
file(GLOB SOURCES src/*.cpp)
file(GLOB TEST_SOURCES tests/*.cpp)

# GoogleTest from Homebrew
include_directories(/opt/homebrew/opt/googletest/include)
link_directories(/opt/homebrew/opt/googletest/lib)

add_executable(test_basic_lru_cache ${SOURCES} ${TEST_SOURCES})
target_link_libraries(test_basic_lru_cache gtest gtest_main pthread)

