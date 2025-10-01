#pragma once

#include <iostream>
#include <unordered_map>
#include <optional>

namespace lru_cache {

template <typename T1, typename T2>
class Node {
public:
    T1 key;
    T2 value;
    Node<T1, T2>* next;
    Node<T1, T2>* prev;

    Node();
    Node(const T1& k, const T2& val);
    void to_string();
};

template <typename T1, typename T2>
class LRUCache {
private:
    Node<T1, T2>* dummy_head;
    Node<T1, T2>* dummy_tail;
    std::unordered_map<T1, Node<T1, T2>*> hashmap;
    int current_capacity;
    int current_size;

    void addToHead(Node<T1, T2>* node);
    void removeNode(Node<T1, T2>* node);
    void moveToHead(Node<T1, T2>* node);
    void evictTail();

public:
    LRUCache(int cap = 10);
    std::optional<T2> get(const T1& key);
    void put(const T1& key, const T2& value);
    int getSize();
    int getCapacity();
    void clear();
    void printCache();
    bool contains(const T1& key);
    bool empty();
    bool full();
};

}

#include "lru_cache.tpp"
