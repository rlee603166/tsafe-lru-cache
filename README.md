# tsafe-lru-cache

A header-only C++20 template library exploring different approaches to thread-safe caching.

## Architecture

The library has four cache implementations, each building on the last.

### LRUCache

Single-threaded LRU cache. Doubly-linked list with a hashmap for O(1) lookup. Most recently used items go to the head, least recently used get evicted from the tail.

### TSafeCache

Wraps `LRUCache` with a `std::shared_mutex`. Reads take a shared lock, writes take a unique lock. Simple but all operations contend on a single lock.

### ShardedLRUCache

Splits keys across 16 `TSafeCache` instances using `std::hash`. Threads hitting different shards don't block each other, which helps under contention.

### ShardedClockCache

Replaces LRU with a CLOCK eviction policy and adds a per-shard bloom filter.

CLOCK uses a circular buffer with reference bits instead of a linked list. On eviction it sweeps the buffer, giving recently accessed entries a second chance before evicting. This is simpler than LRU and more cache-line friendly.

The bloom filter sits in front of each shard and checks whether a key *might* exist before acquiring any lock. If the filter says no, `get()` returns immediately with zero locking. Since ~50% of lookups are misses in typical workloads, this cuts lock contention roughly in half.

The bloom filter is a simple bit array with two hash functions. It doesn't support deletion, so evicted keys stay in the filter as false positives. This is fine for a cache since a false positive just means we take the lock and do a hashmap miss, which is the same as before.

### gRPC Service

A gRPC server wraps `ShardedClockCache<string, string>` so you can talk to the cache over the network. Supports `Get`, `Put`, `Clear`, and `GetStats`. The server tracks total gets and hits with atomics so `GetStats` can report hit rate.

Each benchmark client thread opens its own gRPC channel to avoid HTTP/2 stream contention, uses a spin barrier so all threads start at once, and records per-operation latency for percentile reporting.

Basically a baby Redis: an in-memory key-value store you can hit from any process over the network.

## Build

```bash
make tests          # build test binaries
make test           # build + run all tests
make compare        # in-process benchmark

# gRPC (requires: brew install grpc protobuf)
make grpc           # build cache_server + grpc_benchmark
make clean          # remove all binaries
```

## Usage

```bash
# start server
./cache_server --port 50051 --capacity 10000

# run benchmark against it
./grpc_benchmark --target localhost:50051 --threads 16 --ops 10000
```
