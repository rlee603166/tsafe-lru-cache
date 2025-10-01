
#include <gtest/gtest.h>
#include "tsafe_lru_cache/basic_lru_cache.hpp"
#include <string>
#include <vector>

using namespace std;

class LRUCacheTest : public ::testing::Test {};

TEST_F(LRUCacheTest, Construction) {
    basic_cache::LRUCache<int, string> cache(3);

    EXPECT_EQ(cache.getSize(), 0);
    EXPECT_EQ(cache.getCapacity(), 3);
    EXPECT_TRUE(cache.empty());
}

// Test zero capacity (edge case)
TEST_F(LRUCacheTest, ZeroCapacity) {
    basic_cache::LRUCache<int, string> cache(0);    

    EXPECT_EQ(cache.getSize(), 0);
    EXPECT_EQ(cache.getCapacity(), 0);

    cache.put(1, "100");
    EXPECT_EQ(cache.getSize(), 0);

    auto result = cache.get(1);
    EXPECT_FALSE(result.has_value());
}


// Test single element operation
TEST_F(LRUCacheTest, SingleElement) {
    basic_cache::LRUCache<int, string> cache(1);
    
    // Insert single element
    cache.put(1, "one");
    EXPECT_EQ(cache.getSize(), 1);
    EXPECT_FALSE(cache.empty());
    
    // Retrieve it
    auto result = cache.get(1);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "one");
    
    // Insert another element (should evict the first)
    cache.put(2, "two");
    EXPECT_EQ(cache.getSize(), 1);
    
    // First element should be gone
    auto result1 = cache.get(1);
    EXPECT_FALSE(result1.has_value());
    
    // Second element should be present
    auto result2 = cache.get(2);
    ASSERT_TRUE(result2.has_value());
    EXPECT_EQ(result2.value(), "two");
}

// Test basic get/put operations
TEST_F(LRUCacheTest, BasicOperations) {
    basic_cache::LRUCache<int, string> cache(3);
    
    // Test empty cache get
    auto result = cache.get(1);
    EXPECT_FALSE(result.has_value());
    
    // Insert elements
    cache.put(1, "one");
    cache.put(2, "two");
    cache.put(3, "three");
    
    EXPECT_EQ(cache.getSize(), 3);
    
    // Test successful gets
    auto r1 = cache.get(1);
    auto r2 = cache.get(2);
    auto r3 = cache.get(3);
    
    ASSERT_TRUE(r1.has_value());
    ASSERT_TRUE(r2.has_value());
    ASSERT_TRUE(r3.has_value());
    
    EXPECT_EQ(r1.value(), "one");
    EXPECT_EQ(r2.value(), "two");
    EXPECT_EQ(r3.value(), "three");
}

// Test LRU eviction behavior
TEST_F(LRUCacheTest, LRUEviction) {
    basic_cache::LRUCache<int, string> cache(3);
    
    // Fill cache to capacity
    cache.put(1, "one");
    cache.put(2, "two");
    cache.put(3, "three");
    
    // Access element 1 to make it recently used
    cache.get(1);
    
    // Insert element 4 (should evict element 2, the least recently used)
    cache.put(4, "four");
    
    EXPECT_EQ(cache.getSize(), 3);
    
    // Element 2 should be evicted
    auto r2 = cache.get(2);
    EXPECT_FALSE(r2.has_value());
    
    // Elements 1, 3, 4 should still be present
    auto r1 = cache.get(1);
    auto r3 = cache.get(3);
    auto r4 = cache.get(4);
    
    ASSERT_TRUE(r1.has_value());
    ASSERT_TRUE(r3.has_value());
    ASSERT_TRUE(r4.has_value());
    
    EXPECT_EQ(r1.value(), "one");
    EXPECT_EQ(r3.value(), "three");
    EXPECT_EQ(r4.value(), "four");
}

// Test updating existing keys
TEST_F(LRUCacheTest, UpdateExistingKey) {
    basic_cache::LRUCache<int, string> cache(3);
    
    cache.put(1, "original");
    cache.put(2, "two");
    cache.put(3, "three");
    
    // Update existing key
    cache.put(1, "updated");
    
    // Size should remain the same
    EXPECT_EQ(cache.getSize(), 3);
    
    // Value should be updated
    auto result = cache.get(1);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "updated");
    
    // Key 1 should now be most recently used
    cache.put(4, "four");
    
    // Key 2 should be evicted (was least recently used)
    auto r2 = cache.get(2);
    EXPECT_FALSE(r2.has_value());
    
    // Key 1 should still be present
    auto r1 = cache.get(1);
    ASSERT_TRUE(r1.has_value());
    EXPECT_EQ(r1.value(), "updated");
}

// Test LRU ordering with gets
TEST_F(LRUCacheTest, LRUOrderingWithGets) {
    basic_cache::LRUCache<int, string> cache(3);
    
    cache.put(1, "one");
    cache.put(2, "two");
    cache.put(3, "three");
    
    // Access in order: 1, 2, 3 (making 3 most recent, 1 least recent)
    cache.get(1);
    cache.get(2);
    cache.get(3);
    
    // Insert new element (should evict 1)
    cache.put(4, "four");
    
    auto r1 = cache.get(1);
    EXPECT_FALSE(r1.has_value());  // Should be evicted
    
    // Others should still be present
    auto r2 = cache.get(2);
    auto r3 = cache.get(3);
    auto r4 = cache.get(4);
    
    ASSERT_TRUE(r2.has_value());
    ASSERT_TRUE(r3.has_value());
    ASSERT_TRUE(r4.has_value());
}

// Test sequence of evictions
TEST_F(LRUCacheTest, EvictionSequence) {
    basic_cache::LRUCache<int, string> cache(2);
    
    // Insert sequence: 1, 2, 3, 4, 5
    cache.put(1, "one");
    cache.put(2, "two");     // Cache: [1, 2]
    cache.put(3, "three");   // Cache: [2, 3] (1 evicted)
    cache.put(4, "four");    // Cache: [3, 4] (2 evicted)
    cache.put(5, "five");    // Cache: [4, 5] (3 evicted)
    
    EXPECT_EQ(cache.getSize(), 2);
    
    // Only 4 and 5 should be present
    EXPECT_FALSE(cache.get(1).has_value());
    EXPECT_FALSE(cache.get(2).has_value());
    EXPECT_FALSE(cache.get(3).has_value());
    
    auto r4 = cache.get(4);
    auto r5 = cache.get(5);
    ASSERT_TRUE(r4.has_value());
    ASSERT_TRUE(r5.has_value());
    EXPECT_EQ(r4.value(), "four");
    EXPECT_EQ(r5.value(), "five");
}

// Test clear functionality
TEST_F(LRUCacheTest, Clear) {
    basic_cache::LRUCache<int, string> cache(3);
    
    cache.put(1, "one");
    cache.put(2, "two");
    cache.put(3, "three");
    
    EXPECT_EQ(cache.getSize(), 3);
    EXPECT_FALSE(cache.empty());
    
    cache.clear();
    
    EXPECT_EQ(cache.getSize(), 0);
    EXPECT_TRUE(cache.empty());
    
    // All elements should be gone
    EXPECT_FALSE(cache.get(1).has_value());
    EXPECT_FALSE(cache.get(2).has_value());
    EXPECT_FALSE(cache.get(3).has_value());
    
    // Should be able to use cache normally after clear
    cache.put(4, "four");
    auto r4 = cache.get(4);
    ASSERT_TRUE(r4.has_value());
    EXPECT_EQ(r4.value(), "four");
}

// Test with different key/value types
TEST_F(LRUCacheTest, DifferentTypes) {
    basic_cache::LRUCache<string, int> cache(2);
    
    cache.put("hello", 42);
    cache.put("world", 84);
    
    auto r1 = cache.get("hello");
    auto r2 = cache.get("world");
    
    ASSERT_TRUE(r1.has_value());
    ASSERT_TRUE(r2.has_value());
    EXPECT_EQ(r1.value(), 42);
    EXPECT_EQ(r2.value(), 84);
    
    // Test eviction with string keys
    cache.put("foo", 100);
    
    // "hello" should be evicted
    auto r3 = cache.get("hello");
    EXPECT_FALSE(r3.has_value());
    
    auto r4 = cache.get("foo");
    ASSERT_TRUE(r4.has_value());
    EXPECT_EQ(r4.value(), 100);
}

// Test large capacity
TEST_F(LRUCacheTest, LargeCapacity) {
    const size_t capacity = 1000;
    basic_cache::LRUCache<int, int> cache(capacity);
    
    // Fill cache completely
    for (int i = 0; i < static_cast<int>(capacity); ++i) {
        cache.put(i, i * 2);
    }
    
    EXPECT_EQ(cache.getSize(), capacity);
    
    // All elements should be retrievable
    for (int i = 0; i < static_cast<int>(capacity); ++i) {
        auto result = cache.get(i);
        ASSERT_TRUE(result.has_value()) << "Failed to get key: " << i;
        EXPECT_EQ(result.value(), i * 2);
    }
    
    // Insert one more element
    cache.put(1000, 2000);
    EXPECT_EQ(cache.getSize(), capacity);
    
    // Key 0 should be evicted
    auto r0 = cache.get(0);
    EXPECT_FALSE(r0.has_value());
    
    // Key 1000 should be present
    auto r1000 = cache.get(1000);
    ASSERT_TRUE(r1000.has_value());
    EXPECT_EQ(r1000.value(), 2000);
}

// Test stress scenario with many operations
TEST_F(LRUCacheTest, StressTest) {
    basic_cache::LRUCache<int, string> cache(10);
    
    vector<int> keys_to_test;
    
    for (int i = 0; i < 100; ++i) {
        cache.put(i, "value_" + to_string(i));
        
        if (i % 10 == 9) {
            for (int j = max(0, i - 4); j <= i; ++j) {
                auto result = cache.get(j);
                if (result.has_value()) {
                    EXPECT_EQ(result.value(), "value_" + to_string(j));
                    keys_to_test.push_back(j);
                }
            }
        }
    }
    
    EXPECT_EQ(cache.getSize(), 10);
    
    for (int i = 90; i < 100; ++i) {
        auto result = cache.get(i);
        ASSERT_TRUE(result.has_value()) << "Key " << i << " should be present";
        EXPECT_EQ(result.value(), "value_" + to_string(i));
    }
}

// Test copy/move semantics (if implemented)
// TEST_F(LRUCacheTest, CopyMoveSemantics) {
//     basic_cache::LRUCache<int, string> cache1(3);
//     cache1.put(1, "one");
//     cache1.put(2, "two");
//     
//     // Test copy construction
//     // basic_cache::LRUCache<int, string> cache2 = cache1;
//     
//     // Test move construction
//     // basic_cache::LRUCache<int, string> cache3 = std::move(cache1);
// }

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
