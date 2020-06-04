/*
 * extendible_hash.h : implementation of in-memory hash table using extendible
 * hashing
 *
 * Functionality: The buffer pool manager must maintain a page table to be able
 * to quickly map a PageId to its corresponding memory location; or alternately
 * report that the PageId does not match any currently-buffered page.
 */

#pragma once

#include <cstdlib>
#include <vector>
#include <string>
#include <array>
#include <functional>
#include <algorithm>
#include <memory>
#include <mutex>
#include <shared_mutex>

#include "hash/hash_table.h"
#include "common/rwmutex.h"

namespace cmudb {

// Pre-computed mask array to get the last n bits of an int
// Usage Example: value & LAST_N_BIT_MASK[2] ==> value & 0x3 ==> value & 0b11 => get the last two bits of value
// This array should/must fit into L1 cache, so we can "get the last bits of an int" in SINGLE bit operator (&)
// Alternative we can use calculation like: value & ((1 << n) - 1) to get the same job done.
static constexpr std::array<size_t, 32> LAST_N_BITS_MASK { 0,
                                                           0x1,        0x3,        0x7,       0xF,
                                                           0x1F,       0x3F,       0x7F,      0xFF,
                                                           0x1FF,      0x3FF,      0x7FF,     0xFFF,
                                                           0x1FFF,     0x3FFF,     0x7FFF,    0xFFFF,
                                                           0x1FFFF,    0x3FFFF,    0x7FFFF,   0xFFFFF,
                                                           0x1FFFFF,   0x3FFFFF,   0x7FFFFF,  0xFFFFFF,
                                                           0x1FFFFFF,  0x3FFFFFF,  0x7FFFFFF, 0xFFFFFFF,
                                                           0x1FFFFFFF, 0x3FFFFFFF, 0x7FFFFFFF };

__always_inline static int GET_LAST_N_BITS(size_t value, int n) { return value & LAST_N_BITS_MASK[n]; }

// Pre-computed mask array to get the last n-th bit of an int
// Usage Example: value & LAST_N_BIT_MASK[2] ==> value & 0x2 ==> value & 0b10 => get the last 2rd bit of value
// This array should/must fit into L1 cache, so we can "get the last bits of an int" in SINGLE bit operator (&)
// Alternative we can use calculation like: value & ((1 << (n - 1))) to get the same job done.
static constexpr std::array<size_t, 32> LAST_N_TH_BIT_MASK { 0,
                                                             0x1,        0x2,        0x4,       0x8,
                                                             0x10,       0x20,       0x40,      0x80,
                                                             0x100,      0x200,      0x400,     0x800,
                                                             0x1000,     0x2000,     0x4000,    0x8000,
                                                             0x10000,    0x20000,    0x40000,   0x80000,
                                                             0x100000,   0x200000,   0x400000,  0x800000,
                                                             0x1000000,  0x2000000,  0x4000000, 0x8000000,
                                                             0x10000000, 0x20000000, 0x40000000 };

__always_inline static int GET_LAST_N_TH_BIT(size_t value, int n) { return value & LAST_N_TH_BIT_MASK[n]; }

template <typename K, typename V>
class ExtendibleHash : public HashTable<K, V> {
public:
  // constructor
  ExtendibleHash(size_t size);
  // helper function to generate hash addressing
  size_t HashKey(const K &key) const;
  // helper function to get global & local depth
  int GetGlobalDepth() const;
  int GetLocalDepth(int bucket_id) const;
  int GetNumBuckets() const;

  // lookup and modifier
  bool Find(const K &key, V &value) override;
  bool Remove(const K &key) override;
  void Insert(const K &key, const V &value) override;

  // get the size of hash table, added by Jigao
  __always_inline size_t GetSize() override {
    std::shared_lock<std::shared_mutex> shared_lock(global_latch);
    return size;
  }


private:

  struct Bucket {
    std::vector<K> keys;
    std::vector<V> values;
    int localDepth = 0;
    std::shared_mutex bucket_latch;

    // constructor
    Bucket() = default;

    Bucket(int localDepth) : localDepth(localDepth) {};

    /**
     * lookup function to find value associate with input key
     * NOT THREAD SAFE, MUST PROTECTED BY LATCH
     */
    inline bool Find(const K &key, V &value) const {
      const auto it = std::find(keys.cbegin(), keys.cend(), key);
      if (it == keys.end()) {
        return false;
      } else {
        const auto match_index = std::distance(keys.cbegin(), it);
        value = values[match_index];
        return true;
      }
    }

    /**
     * check the existence of a key
     * if exists, return true and UPDATE THE VALUE. Otherwise, return false
     * NOT THREAD SAFE, MUST PROTECTED BY LATCH
     */
    inline bool Exist(const K &key, const V &value) {
      const auto it = std::find(keys.cbegin(), keys.cend(), key);
      if (it == keys.end()) {
        return false;
      } else {
        const auto match_index = std::distance(keys.cbegin(), it);
        values[match_index] = value;
        return true;
      }
    }

    /**
     * delete <key,value> entry in bucket
     * NOT THREAD SAFE, MUST PROTECTED BY LATCH
     */
    inline bool Remove(const K &key) {
      const auto it = std::find(keys.cbegin(), keys.cend(), key);
      if (it == keys.end()) {
        return false;
      } else {
        const auto match_index = std::distance(keys.cbegin(), it);
        keys[match_index] = keys.back();
        keys.pop_back();
        values[match_index] = values.back();
        values.pop_back();
        return true;
      }
    }

    /**
     * insert <key,value> entry in bucket
     * NOT THREAD SAFE, MUST PROTECTED BY LATCH
     */
    inline void Insert(const K &key, const V &value) {
      keys.emplace_back(key);
      values.emplace_back(value);
    }
  };

  // the element number limitation in a single bucket
  const size_t bucketSize = 0;

  // global depth
  int globalDepth = 0;

  // number of buckets
  int numBuckets = 1;

  // number of keys
  size_t size = 0;

  // Bucket Address Table a.k.a Directory
  std::vector<std::shared_ptr<Bucket>> bucketAddressTable = {};

  mutable std::shared_mutex global_latch;

  /**
   * helper function to return pointer to bucket of the key
   * NOT THREAD SAFE, MUST PROTECTED BY LATCH
   * @param key
   * @return a shared point of the bucket, where the key lands
   */
  inline std::shared_ptr<Bucket> GetBucket(const K &key) const {
    return bucketAddressTable[GET_LAST_N_BITS(HashKey(key), globalDepth)];
  }

  /**
   * check the existence of a key and update the value, if the key exists
   * NOT THREAD SAFE, MUST PROTECTED BY LATCH
   * @param key
   * @param value
   * @return true for the key exists, otherwise false
   */
  inline bool Exists(const K &key, const V &value);

  /**
   * double the size of bucket address table
   * NOT THREAD SAFE, MUST PROTECTED BY LATCH
   */
  inline void Grow() {
    for (size_t it = 0, old_size = bucketAddressTable.size(); it != old_size; it++) {
      bucketAddressTable.emplace_back(bucketAddressTable[it]);
    }
  }
};
} // namespace cmudb
