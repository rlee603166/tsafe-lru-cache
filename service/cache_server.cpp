#include <atomic>
#include <iostream>
#include <string>

#include <grpcpp/grpcpp.h>
#include "cache_service.grpc.pb.h"
#include "tsafe_lru_cache/sharded_clock.hpp"

class CacheServiceImpl final : public cache::CacheService::Service {
    sharded_clock::ShardedClockCache<std::string, std::string> cache_;
    std::atomic<uint64_t> gets_{0}, hits_{0};

public:
    explicit CacheServiceImpl(int cap) : cache_(cap) {}

    grpc::Status Get(grpc::ServerContext*, const cache::GetRequest* req,
                     cache::GetResponse* resp) override {
        gets_.fetch_add(1, std::memory_order_relaxed);
        if (auto v = cache_.get(req->key())) {
            hits_.fetch_add(1, std::memory_order_relaxed);
            resp->set_found(true);
            resp->set_value(*v);
        }
        return grpc::Status::OK;
    }

    grpc::Status Put(grpc::ServerContext*, const cache::PutRequest* req,
                     cache::PutResponse*) override {
        cache_.put(req->key(), req->value());
        return grpc::Status::OK;
    }

    grpc::Status Clear(grpc::ServerContext*, const cache::ClearRequest*,
                       cache::ClearResponse*) override {
        cache_.clear();
        return grpc::Status::OK;
    }

    grpc::Status GetStats(grpc::ServerContext*, const cache::StatsRequest*,
                          cache::StatsResponse* resp) override {
        auto g = gets_.load(std::memory_order_relaxed);
        auto h = hits_.load(std::memory_order_relaxed);
        resp->set_size(cache_.getSize());
        resp->set_capacity(cache_.getCapacity());
        resp->set_total_gets(g);
        resp->set_total_hits(h);
        resp->set_hit_rate(g > 0 ? double(h) / g : 0.0);
        return grpc::Status::OK;
    }
};

int main(int argc, char** argv) {
    int port = 50051, capacity = 10000;
    for (int i = 1; i + 1 < argc; i += 2) {
        std::string arg = argv[i];
        if (arg == "--port") port = std::stoi(argv[i + 1]);
        else if (arg == "--capacity") capacity = std::stoi(argv[i + 1]);
    }

    auto addr = "0.0.0.0:" + std::to_string(port);
    CacheServiceImpl service(capacity);

    grpc::ServerBuilder builder;
    builder.AddListeningPort(addr, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    auto server = builder.BuildAndStart();
    std::cout << "Listening on " << addr << " (capacity=" << capacity << ")\n";
    server->Wait();
}
