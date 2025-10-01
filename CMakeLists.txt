cmake_minimum_required(VERSION 3.20)

project(TSafeLRUCache
    VERSION 1.0.0
    DESCRIPTION "Thread-Safe LRU Cache Implementation"
    LANGUAGES CXX
)

# =====================================================
# Global Configuration
# =====================================================

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler-specific options
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(-g -O0 -fsanitize=address)
        add_link_options(-fsanitize=address)
    else()
        add_compile_options(-O3)
    endif()
endif()

# =====================================================
# Dependencies
# =====================================================

# Required for testing
find_package(GTest QUIET)
if(NOT GTest_FOUND)
    message(STATUS "GTest not found via find_package, using Homebrew fallback")
    set(GTEST_ROOT "/opt/homebrew/opt/googletest")
    if(EXISTS "${GTEST_ROOT}")
        include_directories("${GTEST_ROOT}/include")
        link_directories("${GTEST_ROOT}/lib")
        set(GTEST_LIBRARIES gtest gtest_main pthread)
        set(GTest_FOUND TRUE)
    endif()
endif()

# Optional for benchmarking (Phase 4)
find_package(benchmark QUIET)
if(benchmark_FOUND)
    message(STATUS "Google Benchmark found - benchmarking enabled")
    set(BENCHMARKING_ENABLED TRUE)
else()
    message(STATUS "Google Benchmark not found - benchmarking disabled")
    set(BENCHMARKING_ENABLED FALSE)
endif()

# =====================================================
# Header-Only Library Target
# =====================================================

add_library(tsafe_lru_cache INTERFACE)
add_library(TSafeLRUCache::tsafe_lru_cache ALIAS tsafe_lru_cache)

target_include_directories(tsafe_lru_cache INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_compile_features(tsafe_lru_cache INTERFACE cxx_std_20)

# =====================================================
# Examples
# =====================================================

option(BUILD_EXAMPLES "Build example programs" ON)

if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# =====================================================
# Testing
# =====================================================

option(BUILD_TESTING "Build test programs" ON)

if(BUILD_TESTING AND GTest_FOUND)
    enable_testing()
    add_subdirectory(tests)
else()
    message(WARNING "Testing disabled - GTest not found")
endif()

# =====================================================
# Benchmarking
# =====================================================

option(BUILD_BENCHMARKS "Build benchmark programs" OFF)

if(BUILD_BENCHMARKS AND BENCHMARKING_ENABLED)
    add_subdirectory(benchmarks)
endif()

# =====================================================
# Installation (for library use)
# =====================================================

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(TARGETS tsafe_lru_cache
    EXPORT TSafeLRUCacheTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(EXPORT TSafeLRUCacheTargets
    FILE TSafeLRUCacheTargets.cmake
    NAMESPACE TSafeLRUCache::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/TSafeLRUCache
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/TSafeLRUCacheConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/TSafeLRUCacheConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/TSafeLRUCache
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/TSafeLRUCacheConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/TSafeLRUCacheConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/TSafeLRUCacheConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/TSafeLRUCache
)

# =====================================================
# Development Tools
# =====================================================

# Add custom targets for development
add_custom_target(format
    COMMAND find ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/examples ${CMAKE_SOURCE_DIR}/tests
            -name "*.hpp" -o -name "*.cpp" -o -name "*.tpp" | xargs clang-format -i
    COMMENT "Formatting source code"
    VERBATIM
)

add_custom_target(clean-build
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}
    COMMENT "Cleaning build directory"
    VERBATIM
)

# =====================================================
# Status Report
# =====================================================

message(STATUS "")
message(STATUS "=====================================================")
message(STATUS "  ${PROJECT_NAME} v${PROJECT_VERSION} Configuration")
message(STATUS "=====================================================")
message(STATUS "  Build type:          ${CMAKE_BUILD_TYPE}")
message(STATUS "  Compiler:            ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  C++ Standard:        ${CMAKE_CXX_STANDARD}")
message(STATUS "")
message(STATUS "  Build Examples:      ${BUILD_EXAMPLES}")
message(STATUS "  Build Testing:       ${BUILD_TESTING}")
message(STATUS "  Build Benchmarks:    ${BUILD_BENCHMARKS}")
message(STATUS "")
message(STATUS "  GTest found:         ${GTest_FOUND}")
message(STATUS "  Benchmark found:     ${BENCHMARKING_ENABLED}")
message(STATUS "=====================================================")
message(STATUS "")