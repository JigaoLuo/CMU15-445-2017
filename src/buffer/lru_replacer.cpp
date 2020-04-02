/**
 * LRU implementation
 */
#include <cassert>

#include "buffer/lru_replacer.h"
#include "page/page.h"

namespace cmudb {

template <typename T> LRUReplacer<T>::LRUReplacer() {}

template <typename T> LRUReplacer<T>::~LRUReplacer() {}

/*
 * Insert value into LRU
 */
template <typename T> void LRUReplacer<T>::Insert(const T &value) {
  std::lock_guard<std::shared_mutex> lockGuard(latch);
  assert(ht.size() == lru_list.size());

  // 1. Check if the value already there
  const auto got = ht.find(value);
  if (got == ht.end()) {
    // 2.1 value no there, then insert it
    lru_list.emplace_front(value);
    ht.emplace(value, lru_list.begin());
  } else {
    // 2.2 value already there, adjust the LRU list
    lru_list.splice(lru_list.begin(), lru_list, got->second);
  }
}

/* If LRU is non-empty, pop the head member from LRU to argument "value", and
 * return true. If LRU is empty, return false
 */
template <typename T> bool LRUReplacer<T>::Victim(T &value) {
  std::lock_guard<std::shared_mutex> lockGuard(latch);
  // 0. Check the size, if any can be victimized
  if (ht.empty()) return false;
  assert(ht.size() == lru_list.size());

  // 1. Pop from the end of the list
  value = lru_list.back();
  lru_list.pop_back();

  // 2. Erase from the hash table
  ht.erase(value);
  return true;
}

/*
 * Remove value from LRU. If removal is successful, return true, otherwise
 * return false
 */
template <typename T> bool LRUReplacer<T>::Erase(const T &value) {
  std::lock_guard<std::shared_mutex> lockGuard(latch);
  assert(ht.size() == lru_list.size());

  // 1. Check if the value already there
  const auto got = ht.find(value);
  if (got == ht.end()) {
    // 2.1 value no there, do nothing
    return false;
  } else {
    // 2.2 value already there, erase from the LRU list and the hash table
    lru_list.erase(got->second);
    ht.erase(got);
    return true;
  }
}

template <typename T> size_t LRUReplacer<T>::Size() const {
  std::shared_lock<std::shared_mutex> lockGuard(latch);
  assert(ht.size() == lru_list.size());
  return ht.size();
}

template class LRUReplacer<Page *>;
// test only
template class LRUReplacer<int>;

} // namespace cmudb
