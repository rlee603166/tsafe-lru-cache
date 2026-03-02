CXX      := c++
CXXFLAGS := -std=c++20 -Wall -Wextra -O2 -Iinclude
GTEST    := -I/opt/homebrew/opt/googletest/include -L/opt/homebrew/opt/googletest/lib -lgtest -lgtest_main -pthread

TESTS := test_basic_lru test_tsafe_cache test_sharded_cache test_clock_cache
GRPC  := cache_server grpc_benchmark

.PHONY: all tests grpc clean test

all: tests grpc

# --- Tests ---
tests: $(TESTS)

test_basic_lru: tests/test_basic_lru_cache.cpp
	$(CXX) $(CXXFLAGS) $< -o $@ $(GTEST)

test_%: tests/test_%.cpp
	$(CXX) $(CXXFLAGS) $< -o $@ $(GTEST)

test: tests
	@for t in $(TESTS); do echo "=== $$t ===" && ./$$t && echo || exit 1; done

# --- Benchmark (in-process, no gRPC) ---
compare: benchmarks/compare.cpp
	$(CXX) $(CXXFLAGS) -pthread $< -o $@

# --- gRPC ---
GRPC_CFLAGS := $(shell pkg-config --cflags grpc++ protobuf 2>/dev/null)
GRPC_LIBS   := $(shell pkg-config --libs grpc++ protobuf 2>/dev/null)
PROTOC      := protoc
GRPC_PLUGIN := $(shell which grpc_cpp_plugin 2>/dev/null)
GEN         := gen

grpc: $(GRPC)

$(GEN)/cache_service.pb.cc $(GEN)/cache_service.grpc.pb.cc: proto/cache_service.proto
	@mkdir -p $(GEN)
	$(PROTOC) --proto_path=proto --cpp_out=$(GEN) \
		--grpc_out=$(GEN) --plugin=protoc-gen-grpc=$(GRPC_PLUGIN) $<

cache_server: service/cache_server.cpp $(GEN)/cache_service.pb.cc $(GEN)/cache_service.grpc.pb.cc
	$(CXX) $(CXXFLAGS) $(GRPC_CFLAGS) -I$(GEN) $^ -o $@ $(GRPC_LIBS)

grpc_benchmark: benchmarks/grpc_benchmark.cpp $(GEN)/cache_service.pb.cc $(GEN)/cache_service.grpc.pb.cc
	$(CXX) $(CXXFLAGS) $(GRPC_CFLAGS) -I$(GEN) $^ -o $@ $(GRPC_LIBS)

clean:
	rm -rf $(TESTS) $(GRPC) compare $(GEN)
