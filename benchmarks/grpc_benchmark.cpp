#include <algorithm>
#include <atomic>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <random>
#include <string>
#include <thread>
#include <vector>

#include <grpcpp/grpcpp.h>
#include "cache_service.grpc.pb.h"

using Clock = std::chrono::high_resolution_clock;

int main(int argc, char** argv) {
    std::string target = "localhost:50051";
    int nthreads = 16, ops = 10000, read_pct = 95, key_space = 20000;

    for (int i = 1; i + 1 < argc; i += 2) {
        std::string a = argv[i];
        if (a == "--target") target = argv[i + 1];
        else if (a == "--threads") nthreads = std::stoi(argv[i + 1]);
        else if (a == "--ops") ops = std::stoi(argv[i + 1]);
        else if (a == "--read-pct") read_pct = std::stoi(argv[i + 1]);
        else if (a == "--key-space") key_space = std::stoi(argv[i + 1]);
    }

    std::cout << "target=" << target << " threads=" << nthreads
              << " ops/thread=" << ops << " read%=" << read_pct
              << " keys=" << key_space << "\n";

    // Pre-populate
    {
        auto stub = cache::CacheService::NewStub(
            grpc::CreateChannel(target, grpc::InsecureChannelCredentials()));
        for (int i = 0; i < key_space / 2; ++i) {
            cache::PutRequest req;
            req.set_key("k" + std::to_string(i));
            req.set_value("v" + std::to_string(i));
            cache::PutResponse resp;
            grpc::ClientContext ctx;
            if (auto s = stub->Put(&ctx, req, &resp); !s.ok()) {
                std::cerr << "populate failed: " << s.error_message() << "\n";
                return 1;
            }
        }
    }

    // Per-thread results
    struct Result { std::vector<double> lat; int hits = 0, gets = 0; };
    std::vector<Result> results(nthreads);
    std::atomic<bool> go{false};
    std::vector<std::thread> threads;

    for (int t = 0; t < nthreads; ++t) {
        threads.emplace_back([&, t]() {
            auto stub = cache::CacheService::NewStub(
                grpc::CreateChannel(target, grpc::InsecureChannelCredentials()));
            std::mt19937 gen(t + 1);
            std::uniform_int_distribution<> kdist(0, key_space - 1), odist(0, 99);
            auto& r = results[t];
            r.lat.reserve(ops);

            while (!go.load(std::memory_order_acquire)) {}

            for (int i = 0; i < ops; ++i) {
                auto key = "k" + std::to_string(kdist(gen));
                auto t0 = Clock::now();

                if (odist(gen) < read_pct) {
                    cache::GetRequest req; req.set_key(key);
                    cache::GetResponse resp; grpc::ClientContext ctx;
                    stub->Get(&ctx, req, &resp);
                    r.gets++;
                    if (resp.found()) r.hits++;
                } else {
                    cache::PutRequest req; req.set_key(key); req.set_value("v");
                    cache::PutResponse resp; grpc::ClientContext ctx;
                    stub->Put(&ctx, req, &resp);
                }

                r.lat.push_back(std::chrono::duration<double, std::micro>(Clock::now() - t0).count());
            }
        });
    }

    auto t0 = Clock::now();
    go.store(true, std::memory_order_release);
    for (auto& t : threads) t.join();
    double wall_us = std::chrono::duration<double, std::micro>(Clock::now() - t0).count();

    // Aggregate
    std::vector<double> all;
    int total_hits = 0, total_gets = 0;
    for (auto& r : results) {
        all.insert(all.end(), r.lat.begin(), r.lat.end());
        total_hits += r.hits;
        total_gets += r.gets;
    }
    std::sort(all.begin(), all.end());

    int total = nthreads * ops;
    auto pct = [&](double p) { return all[size_t(p / 100.0 * (all.size() - 1))]; };

    std::cout << std::fixed << std::setprecision(0)
              << "QPS: " << total / (wall_us / 1e6) << "\n"
              << std::setprecision(1)
              << "p50: " << pct(50) << "us  p95: " << pct(95) << "us  p99: " << pct(99) << "us\n"
              << std::setprecision(1)
              << "hit rate: " << (total_gets > 0 ? 100.0 * total_hits / total_gets : 0)
              << "% (" << total_hits << "/" << total_gets << ")\n";
}
