# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is a C++ template-based LRU (Least Recently Used) Cache implementation called `TSafeLRUCache`. The project implements a multi-phase development approach, starting with a basic LRU cache and progressing through thread-safe, sharded, and optimized implementations.

## Build Commands

### Modern CMake Build (Recommended)
```bash
# Configure and build
mkdir -p build && cd build
cmake ..
make -j$(nproc)

# Run examples
./examples/basic_demo

# Run tests
make test
# OR directly run test executable
./tests/unit/test_basic_lru

# Run benchmarks (if Google Benchmark is available)
cmake .. -DBUILD_BENCHMARKS=ON
make -j$(nproc)
./benchmarks/basic_benchmark
```

### Build Options
```bash
# Debug build with sanitizers
cmake .. -DCMAKE_BUILD_TYPE=Debug

# Release build with optimizations
cmake .. -DCMAKE_BUILD_TYPE=Release

# Disable examples/tests
cmake .. -DBUILD_EXAMPLES=OFF -DBUILD_TESTING=OFF

# Enable benchmarking
cmake .. -DBUILD_BENCHMARKS=ON
```

### Legacy Make (Phase 1 only)
```bash
make                    # Build the basic_cache_demo executable
make debug             # Build with debug symbols
make run               # Build and run demo
make clean             # Remove build artifacts
```

## Code Architecture

### Directory Structure
```
include/tsafe_lru_cache/    # Public headers (header-only library)
├── basic_lru_cache.hpp     # Phase 1: Basic LRU cache declarations
└── basic_lru_cache.tpp     # Phase 1: Template implementations

examples/                   # Demo applications
├── basic_demo.cpp          # Phase 1 usage examples
└── CMakeLists.txt

tests/unit/                 # Unit tests
├── test_basic_lru_cache.cpp # Phase 1 comprehensive tests
└── CMakeLists.txt

benchmarks/                 # Performance testing (Phase 4+)
├── basic_benchmark.cpp     # Phase 1 baseline benchmarks
└── CMakeLists.txt
```

### Core Components (Phase 1)

- **`include/tsafe_lru_cache/basic_lru_cache.hpp`**: Template class declarations for `Node<T1,T2>` and `LRUCache<T1,T2>`
- **`include/tsafe_lru_cache/basic_lru_cache.tpp`**: Template implementation file (included by header)
- **`examples/basic_demo.cpp`**: Demo application showing usage examples
- **`tests/unit/test_basic_lru_cache.cpp`**: Comprehensive Google Test suite

### Design Pattern

The LRU cache uses a classic doubly-linked list + hash map design:
- Hash map provides O(1) key-to-node lookup (`std::unordered_map<T1, Node<T1, T2>*>`)
- Doubly-linked list maintains LRU order with dummy head/tail sentinels
- Template-based for type flexibility (`basic_cache::LRUCache<KeyType, ValueType>`)

### Key Operations
- `get(key)`: Returns `std::optional<T2>` for safe access
- `put(key, value)`: Insert/update with automatic LRU eviction
- Utility methods: `empty()`, `full()`, `contains()`, `clear()`, `getSize()`, `getCapacity()`

## Multi-Phase Development Plan

**Phase 1** ✅: Basic single-threaded LRU cache (COMPLETE)
**Phase 2**: Thread-safe version with `std::shared_mutex`
**Phase 3**: Sharded implementation for reduced lock contention
**Phase 4**: Comprehensive performance testing and benchmarking
**Phase 5**: Bloom filter optimization for negative lookups
**Phase 6**: gRPC service layer
**Phase 7**: Production features (monitoring, configuration)

## Testing

### Unit Tests
- Uses Google Test framework (auto-detected via find_package or Homebrew fallback)
- Comprehensive coverage: basic operations, edge cases, LRU eviction behavior
- Different template type combinations and stress testing
- Located in `tests/unit/`

### Benchmarking
- Uses Google Benchmark framework (optional dependency)
- Performance baselines for comparison between phases
- Cache size scaling analysis
- Mixed workload simulation (70% reads, 30% writes)

## Dependencies

### Required
- **C++20** standard (for concepts, ranges if needed in later phases)
- **CMake 3.20+** for modern build system

### Optional
- **Google Test**: For unit testing (auto-detected or Homebrew fallback)
- **Google Benchmark**: For performance testing (Phase 4+)
- **gRPC + Protocol Buffers**: For service layer (Phase 6)

### Homebrew Fallback Paths
- GTest: `/opt/homebrew/opt/googletest/`
- Other dependencies will be added as phases progress

## Development Tools

```bash
# Format code (requires clang-format)
make format

# Clean build directory
make clean-build

# Install library system-wide
make install
```