
#include <iostream>
#include <optional>

namespace basic_cache {

template <typename T1, typename T2>
class Node {
public:
    T1 key;
    T2 value;

    Node<T1, T2>* next;
    Node<T1, T2>* prev;

    Node() : next(nullptr), prev(nullptr) {}

    Node(const T1& k, const T2& val)
        : key(k), value(val), next(nullptr), prev(nullptr) {} 

    void to_string() {
        std::cout << "Node(key: " << key << ", value: " << value << ")";
    }
};

template <typename T1, typename T2> 
class LRUCache {
private:
    Node<T1, T2>* dummy_head;
    Node<T1, T2>* dummy_tail;
    std::unordered_map<T1, Node<T1, T2>*> hashmap;
    int current_capacity;
    int current_size;

    void addToHead(Node<T1, T2>* node) {
        // insert to head
        dummy_head->next->prev = node;
        node->next = dummy_head->next;

        dummy_head->next = node;
        node->prev = dummy_head;
    }

    void removeNode(Node<T1, T2>* node) {
        // remove from list
        node->prev->next = node->next;
        node->next->prev = node->prev;
    }

    void moveToHead(Node<T1, T2>* node) {
        removeNode(node);
        addToHead(node);
    }

    void evictTail() {
        Node<T1, T2>* tail = dummy_tail->prev;

        dummy_tail->prev->prev->next = dummy_tail;
        dummy_tail->prev = dummy_tail->prev->prev;
        current_size--;

        hashmap.erase(tail->key);
        delete tail;
    }

public: 

    LRUCache(int cap = 10) {
        current_capacity = cap;
        current_size = 0;

        dummy_head = new Node<T1, T2>();
        dummy_tail = new Node<T1, T2>();

        dummy_head->next = dummy_tail;
        dummy_tail->prev = dummy_head;
    }


    std::optional<T2> get(const T1& key) {
        // Find node based on key, and move to front
        auto it = hashmap.find(key);
        if (it == hashmap.end())
            return std::nullopt;

        Node<T1, T2>* found = it->second;
        moveToHead(found);
        return found->value;
   }

    void put(const T1& key, const T2& value) {
        // insert at head
        Node<T1, T2>* temp;
        if (hashmap.count(key) != 0) {
            temp = hashmap[key];
            moveToHead(temp);
        } else {
            Node<T1, T2>* temp = new Node<T1, T2>(key, value);
            hashmap[key] = temp;
            addToHead(temp);
            current_size++;

            if (current_size > current_capacity)
                evictTail();
        }
    }

    int getSize() {
        return current_size;
    }

    int getCapacity() {
        return current_capacity;
    }

    void clear() {
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

    void printCache() {
        Node<T1, T2>* temp = dummy_head->next;
        std::cout << "Cache contents (MRU -> LRU): ";
        while (temp != dummy_tail) {
            temp->to_string();
            if (temp->next != dummy_tail) {
                std::cout << " <-> ";
            }
            temp = temp->next;
        }
        std::cout << std::endl;
    }

    bool contains(const T1& key) {
        return hashmap.find(key) != hashmap.end();
    }

    bool empty() {
        return current_size == 0;
    }

    bool full() {
        return current_size == current_size;
    }
};

};
