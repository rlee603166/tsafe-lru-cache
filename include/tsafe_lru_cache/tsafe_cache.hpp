
#include "lru_cache/basic_lru_cache.hpp"
#inclue "mutex"

namespace tsafe_cache {

<template T1, template T2>
class TSafeCache {
private:

    mutable std::shared_mutex mtx_;
    lru_cache::LRUCache<T1, T2>* cache_;

public:

    TSafeCache(int cap = 10);
    void put(const T1& key);
    std::optional<T2> get(const T1& key);
    bool contains(const T1& key);
    void clear();
    int getCapacity();
    int getSize();
    bool empty();
    bool full();
    void printCache();

};

}

#include "tsafe_cache.tpp"
