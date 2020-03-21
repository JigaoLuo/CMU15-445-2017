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
#include <memory>
#include <mutex>

#include "hash/hash_table.h"

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


private:
  // add your own member variables here

  struct Bucket {
    std::vector<K> keys;
    std::vector<V> values;
    int localDepth = 0;

    // constructor
    Bucket() {};

    Bucket(int localDepth) : localDepth(localDepth) {};
  };

  int bucketSize = 0;

  int globalDepth = 0;

  int numBuckets = 0;

  // Bucket Address Table a.k.a Directory
  std::vector<Bucket*> bucketAddressTable = {};

  std::mutex latch;

  /*
   * helper function to return Const Pointer to bucket of the key
   */
  __always_inline
  Bucket* GetBucket(const K &key) const {
    return bucketAddressTable[GET_LAST_N_BITS(HashKey(key), globalDepth)];
  }

  bool Exists(const K &key, const V &value);
};
} // namespace cmudb
