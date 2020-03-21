#include <list>
#include <functional>
#include <algorithm>
#include <cassert>

#include "hash/extendible_hash.h"
#include "page/page.h"

namespace cmudb {

/*
 * constructor
 * array_size: fixed array size for each bucket
 */
template <typename K, typename V>
ExtendibleHash<K, V>::ExtendibleHash(size_t size) {
  bucketSize = size;

  // init bucket address table with the first bucket
  auto bucket = new Bucket();
  numBuckets++;
  bucketAddressTable.emplace_back(bucket);
}

/*
 * helper function to calculate the hashing address of input key
 */
template <typename K, typename V>
size_t ExtendibleHash<K, V>::HashKey(const K &key) const {
  return std::hash<K>{}(key);
}

/*
 * helper function to return global depth of hash table
 * NOTE: you must implement this function in order to pass test
 */
template <typename K, typename V>
int ExtendibleHash<K, V>::GetGlobalDepth() const {
  return globalDepth;
}

/*
 * helper function to return local depth of one specific bucket
 * NOTE: you must implement this function in order to pass test
 */
template <typename K, typename V>
int ExtendibleHash<K, V>::GetLocalDepth(int bucket_id) const {
  return bucketAddressTable[bucket_id]->localDepth;
}

/*
 * helper function to return current number of bucket in hash table
 */
template <typename K, typename V>
int ExtendibleHash<K, V>::GetNumBuckets() const {
  return numBuckets;
}

/*
 * lookup function to find value associate with input key
 */
template <typename K, typename V>
bool ExtendibleHash<K, V>::Find(const K &key, V &value) {
  const auto bucket = GetBucket(key);
  const auto& keys = bucket->keys;
  const auto it = std::find(keys.cbegin(), keys.cend(), key);
  if (it == keys.end()) {
    return false;
  } else {
    const auto match_index = std::distance(keys.cbegin(), it);
    value = bucket->values[match_index];
    return true;
  }
}

/*
 * delete <key,value> entry in hash table
 * Shrink & Combination is not required for this project
 */
template <typename K, typename V>
bool ExtendibleHash<K, V>::Remove(const K &key) {
  const auto bucket = GetBucket(key);
  auto& keys = bucket->keys;
  const auto it = std::find(keys.cbegin(), keys.cend(), key);
  if (it == keys.end()) {
    return false;
  } else {
    const auto match_index = std::distance(keys.cbegin(), it);
    auto& values = bucket->values;
    keys[match_index] = keys.back();
    keys.pop_back();
    values[match_index] = values.back();
    values.pop_back();
    return true;
  }
}

/*
 * exists function to check the existence of a key
 * if exists, return true and UPDATE THE VALUE
 * if not exists, return false
 */
template <typename K, typename V>
bool ExtendibleHash<K, V>::Exists(const K &key, const V &value) {
  auto bucket_j = GetBucket(key);
  auto& keys_j = bucket_j->keys;
  auto& values_j = bucket_j->values;

  const auto it = std::find(keys_j.cbegin(), keys_j.cend(), key);
  if (it != keys_j.end()) {
    const auto match_index = std::distance(keys_j.cbegin(), it);
    values_j[match_index] = value;
    return true;
  } else {
    return false;
  }
}

/*
 * insert <key,value> entry in hash table
 * Split & Redistribute bucket when there is overflow and if necessary increase
 * global depth
 */
template <typename K, typename V>
void ExtendibleHash<K, V>::Insert(const K &key, const V &value) {
  // 0. Check the existence
  if (Exists(key, value)) return;

  // 1. Look up the correspond bucket
  check:
  auto bucket_j = GetBucket(key);
  auto& keys_j = bucket_j->keys;
  auto& values_j = bucket_j->values;
  assert(keys_j.size() == values_j.size());
  assert(keys_j.size() <= bucketSize);

  // 2. Check the bucket, if is full
  if (keys_j.size() == bucketSize) {
    // 2.１ Bucket full
    const auto last_n_bit_global = GET_LAST_N_BITS(HashKey(key), globalDepth);
    const auto last_n_bit_local = GET_LAST_N_BITS(last_n_bit_global, bucket_j->localDepth);
    const auto old_localDepth = bucket_j->localDepth;

    // 2.1.1 Compare the global depth (i) and local depth (i_j)
    if (globalDepth == bucket_j->localDepth) {
      // 2.1.1.1 i == i_j, only one entry in the bucket address table points to bucket j

      // 2.1.1.1.1 increase the size of the bucket address table.
      //           It replaces each entry by two entries, both of which contain the same pointer as the original entry.
      globalDepth++;
      const auto old_bat_size = bucketAddressTable.size();
      for (size_t it = 0; it != old_bat_size; it++) {
        Bucket* copy = bucketAddressTable[it];
        bucketAddressTable.emplace_back(copy);
      }
      assert((1 << globalDepth) == bucketAddressTable.size());
    }

    // 2.1.1.2 i > i_j, more than one entry in the bucket address table points to bucket j.
    //                  The system can split bucket $j$ without increasing the size of the bucket address table
    //                  Do Nothing For This Case

    // 2.1.2 The system allocates a new bucket z,
    //         and sets i_j and i_z to the value that results from adding 1 to the original i_j value.
    bucket_j->localDepth++;
    auto bucket_z = new Bucket(bucket_j->localDepth);
    numBuckets++;
    auto& keys_z = bucket_z->keys;
    auto& values_z = bucket_z->values;

    // 2.1.3 Next, to adjust the entries in the bucket address table that previously pointed to bucket $j$
    int base_point = (1 << old_localDepth) + last_n_bit_local;
    const int iter_incr = 1 << (bucket_j->localDepth);
    const size_t iterations = std::max(((globalDepth - bucket_j->localDepth) << 1), 1);
    for (size_t it = 0; it != iterations; it++, base_point+=iter_incr) {
      bucketAddressTable[base_point] = bucket_z;
    }

    // 2.1.4 rehash each record in bucket j, and allocates it either to bucket j or bucket z
    // 2.1.4.1 Find all match index in keys of bucket j
    std::vector<size_t> matches;
    for (size_t it = keys_j.size() - 1; ; --it) {
      if (GET_LAST_N_TH_BIT(HashKey(keys_j[it]), bucket_j->localDepth) != 0) {
        matches.push_back(it);
      }
      if (it == 0) break;
    }

    // 2.1.4.2 Insert matches to bucket z, and remove matches from bucket j
    for (auto& it : matches) {
      keys_z.emplace_back(keys_j[it]);
      values_z.emplace_back(values_j[it]);

      keys_j[it] = keys_j.back();
      keys_j.pop_back();
      values_j[it] = values_j.back();
      values_j.pop_back();
    }

    // 2.1.5 Check if there is place to insert after rehasing
    const bool to_bucket_z = GET_LAST_N_TH_BIT(HashKey(key), bucket_j->localDepth);
    if((matches.empty() && !to_bucket_z) || matches.size() + to_bucket_z > bucketSize) {
      // 2.1.5.1 No place, so repeat again
      goto check;
    }

    // 2.1.5.2 Exists space, insert key and value
    if (to_bucket_z) {
      keys_z.emplace_back(key);
      values_z.emplace_back(value);
    } else {
      keys_j.emplace_back(key);
      values_j.emplace_back(value);
    }
  } else {
    // 2.2 Bucket not full, just simple insert
    keys_j.emplace_back(key);
    values_j.emplace_back(value);
  }

  assert(keys_j.size() == values_j.size());
  assert(keys_j.size() <= bucketSize);
}

template class ExtendibleHash<page_id_t, Page *>;
template class ExtendibleHash<Page *, std::list<Page *>::iterator>;
// test purpose
template class ExtendibleHash<int, std::string>;
template class ExtendibleHash<int, std::list<int>::iterator>;
template class ExtendibleHash<int, int>;
} // namespace cmudb
