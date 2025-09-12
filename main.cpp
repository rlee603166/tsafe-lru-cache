
#include "src/basic_lru_cache.cpp"
#include <iostream>
#include <string>

int main() {
    std::cout << "=== Basic LRU Cache Demo ===\n\n";
    
    // Example 1: String keys, Integer values
    std::cout << "1. String->Int Cache:\n";
    basic_cache::LRUCache<std::string, int> stringIntCache(3);
    
    stringIntCache.put("apple", 100);
    stringIntCache.put("banana", 200);
    stringIntCache.put("cherry", 300);
    stringIntCache.printCache();
    
    // Access apple (moves it to front)
    auto result = stringIntCache.get("apple");
    if (result) {
        std::cout << "Got apple: " << *result << std::endl;
    }
    stringIntCache.printCache();
    
    // Add new item (should evict banana)
    stringIntCache.put("date", 400);
    stringIntCache.printCache();
    
    std::cout << "\n";
    
    // Example 2: Integer keys, String values
    std::cout << "2. Int->String Cache:\n";
    basic_cache::LRUCache<int, std::string> intStringCache(2);
    
    intStringCache.put(1, "first");
    intStringCache.put(2, "second");
    intStringCache.printCache();
    
    // Update existing key
    intStringCache.put(1, "updated_first");
    intStringCache.printCache();
    
    // Add third item (should evict key 2)
    intStringCache.put(3, "third");
    intStringCache.printCache();
    
    std::cout << "\n";
    
    // Example 3: Demonstrating utility methods
    std::cout << "3. Utility Methods Demo:\n";
    basic_cache::LRUCache<char, double> charDoubleCache(5);
    
    std::cout << "Empty: " << std::boolalpha << charDoubleCache.empty() << std::endl;
    std::cout << "Size: " << charDoubleCache.getSize() << std::endl;
    std::cout << "Capacity: " << charDoubleCache.getCapacity() << std::endl;
    
    charDoubleCache.put('A', 1.1);
    charDoubleCache.put('B', 2.2);
    charDoubleCache.put('C', 3.3);
    
    std::cout << "After adding 3 items:\n";
    std::cout << "Empty: " << charDoubleCache.empty() << std::endl;
    std::cout << "Size: " << charDoubleCache.getSize() << std::endl;
    std::cout << "Full: " << charDoubleCache.full() << std::endl;
    
    std::cout << "Contains 'B': " << charDoubleCache.contains('B') << std::endl;
    std::cout << "Contains 'Z': " << charDoubleCache.contains('Z') << std::endl;
    
    charDoubleCache.printCache();
    
    // Clear cache
    charDoubleCache.clear();
    std::cout << "After clear - Size: " << charDoubleCache.getSize() << std::endl;
    
    return 0;
}
