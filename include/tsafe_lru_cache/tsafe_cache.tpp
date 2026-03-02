#pragma once

namespace tsafe_cache {

// ---------- TSafeCache Public Methods ----------

template <typename T1, typename T2>
TSafeCache<T1, T2>::TSafeCache(int cap) {
    cache_ = new lru_cache::LRUCache<T1, T2>(cap);
}

template <typename T1, typename T2>
void TSafeCache<T1, T2>::put(const T1& key, const T2& value) {
    std::unique_lock<std::shared_mutex> lock(mtx_);
    cache_->put(key, value);
}

template <typename T1, typename T2>
std::optional<T2> TSafeCache<T1, T2>::get(const T1& key) {
    std::unique_lock<std::shared_mutex> lock(mtx_);
    return cache_->get(key);
}

template <typename T1, typename T2>
bool TSafeCache<T1, T2>::contains(const T1& key) {
    std::shared_lock<std::shared_mutex> lock(mtx_);
    return cache_->contains(key);
}

template <typename T1, typename T2>
void TSafeCache<T1, T2>::clear() {
    std::unique_lock<std::shared_mutex> lock(mtx_);
    cache_->clear();
}

template <typename T1, typename T2>
int TSafeCache<T1, T2>::getCapacity() {
    std::shared_lock<std::shared_mutex> lock(mtx_);
    return cache_->getCapacity();
}

template <typename T1, typename T2>
int TSafeCache<T1, T2>::getSize() {
    std::shared_lock<std::shared_mutex> lock(mtx_);
    return cache_->getSize();;
}

template <typename T1, typename T2>
bool TSafeCache<T1, T2>::empty() {
    std::shared_lock<std::shared_mutex> lock(mtx_);
    return cache_->empty();
}

template <typename T1, typename T2>
bool TSafeCache<T1, T2>::full() {
    std::shared_lock<std::shared_mutex> lock(mtx_);
    return cache_->full();
}

template <typename T1, typename T2>
void TSafeCache<T1, T2>::printCache() {
    std::shared_lock<std::shared_mutex> lock(mtx_);
    cache_->printCache();
}

}

