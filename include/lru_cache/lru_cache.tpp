#pragma once

namespace lru_cache {

// ---------- Node Implementation ----------

template <typename T1, typename T2>
Node<T1, T2>::Node() : next(nullptr), prev(nullptr) {}

template <typename T1, typename T2>
Node<T1, T2>::Node(const T1& k, const T2& val)
    : key(k), value(val), next(nullptr), prev(nullptr) {}

template <typename T1, typename T2>
void Node<T1, T2>::to_string() {
    std::cout << "Node(key: " << key << ", value: " << value << ")";
}

// ---------- LRUCache Private Helpers ----------

template <typename T1, typename T2>
void LRUCache<T1, T2>::addToHead(Node<T1, T2>* node) {
    dummy_head->next->prev = node;
    node->next = dummy_head->next;

    dummy_head->next = node;
    node->prev = dummy_head;
}

template <typename T1, typename T2>
void LRUCache<T1, T2>::removeNode(Node<T1, T2>* node) {
    node->prev->next = node->next;
    node->next->prev = node->prev;
}

template <typename T1, typename T2>
void LRUCache<T1, T2>::moveToHead(Node<T1, T2>* node) {
    removeNode(node);
    addToHead(node);
}

template <typename T1, typename T2>
void LRUCache<T1, T2>::evictTail() {
    Node<T1, T2>* tail = dummy_tail->prev;

    dummy_tail->prev = tail->prev;
    tail->prev->next = dummy_tail;

    hashmap.erase(tail->key);
    delete tail;
    current_size--;
}

// ---------- LRUCache Public Methods ----------

template <typename T1, typename T2>
LRUCache<T1, T2>::LRUCache(int cap)
    : current_capacity(cap), current_size(0) {
    dummy_head = new Node<T1, T2>();
    dummy_tail = new Node<T1, T2>();
    dummy_head->next = dummy_tail;
    dummy_tail->prev = dummy_head;
}

template <typename T1, typename T2>
std::optional<T2> LRUCache<T1, T2>::get(const T1& key) {
    auto it = hashmap.find(key);
    if (it == hashmap.end())
        return std::nullopt;

    Node<T1, T2>* found = it->second;
    moveToHead(found);
    return found->value;
}

template <typename T1, typename T2>
void LRUCache<T1, T2>::put(const T1& key, const T2& value) {
    Node<T1, T2>* temp;
    if (hashmap.count(key)) {
        temp = hashmap[key];
        temp->value = value;
        moveToHead(temp);
    } else {
        temp = new Node<T1, T2>(key, value);
        hashmap[key] = temp;
        addToHead(temp);
        current_size++;

        if (current_size > current_capacity)
            evictTail();
    }
}

template <typename T1, typename T2>
int LRUCache<T1, T2>::getSize() {
    return current_size;
}

template <typename T1, typename T2>
int LRUCache<T1, T2>::getCapacity() {
    return current_capacity;
}

template <typename T1, typename T2>
void LRUCache<T1, T2>::clear() {
    Node<T1, T2>* current = dummy_head->next;
    while (current != dummy_tail) {
        Node<T1, T2>* next = current->next;
        delete current;
        current = next;
    }
    dummy_head->next = dummy_tail;
    dummy_tail->prev = dummy_head;
    hashmap.clear();
    current_size = 0;
}

template <typename T1, typename T2>
void LRUCache<T1, T2>::printCache() {
    Node<T1, T2>* temp = dummy_head->next;
    std::cout << "Cache contents (MRU -> LRU): ";
    while (temp != dummy_tail) {
        temp->to_string();
        if (temp->next != dummy_tail) std::cout << " <-> ";
        temp = temp->next;
    }
    std::cout << std::endl;
}

template <typename T1, typename T2>
bool LRUCache<T1, T2>::contains(const T1& key) {
    return hashmap.find(key) != hashmap.end();
}

template <typename T1, typename T2>
bool LRUCache<T1, T2>::empty() {
    return current_size == 0;
}

template <typename T1, typename T2>
bool LRUCache<T1, T2>::full() {
    return current_size == current_capacity;
}

}

