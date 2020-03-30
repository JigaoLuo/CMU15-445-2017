#include <list>
#include <cassert>

#include "hash/extendible_hash.h"
#include "page/page.h"

namespace cmudb {

/*
 * constructor
 * array_size: fixed array size for each bucket
 */
template <typename K, typename V>
ExtendibleHash<K, V>::ExtendibleHash(size_t size) : bucketSize(size)  {
  // init bucket address table with the first bucket
  bucketAddressTable.emplace_back(new Bucket());
}

/*
 * helper function to calculate the hashing address of input key
 */
template <typename K, typename V>
size_t ExtendibleHash<K, V>::HashKey(const K &key) const {
  // std::hash<>: assumption already has specialization for type K
  // namespace std have standard specializations for basic types.
  return std::hash<K>{}(key);
}

/*
 * helper function to return global depth of hash table
 * NOTE: you must implement this function in order to pass test
 */
template <typename K, typename V>
int ExtendibleHash<K, V>::GetGlobalDepth() const {
  std::shared_lock<std::shared_mutex> shared_lock(global_latch);
  return globalDepth;
}

/*
 * helper function to return local depth of one specific bucket
 * NOTE: you must implement this function in order to pass test
 */
template <typename K, typename V>
int ExtendibleHash<K, V>::GetLocalDepth(int bucket_id) const {
  std::shared_lock<std::shared_mutex> shared_lock(global_latch);
  return bucketAddressTable[bucket_id]->localDepth;
}

/*
 * helper function to return current number of bucket in hash table
 */
template <typename K, typename V>
int ExtendibleHash<K, V>::GetNumBuckets() const {
  std::shared_lock<std::shared_mutex> shared_lock(global_latch);
  return numBuckets;
}

/*
 * lookup function to find value associate with input key
 */
template <typename K, typename V>
bool ExtendibleHash<K, V>::Find(const K &key, V &value) {
  std::shared_lock<std::shared_mutex> global_lock(global_latch);
  const auto bucket = GetBucket(key);
  std::shared_lock<std::shared_mutex> bucket_lock_guard(bucket->bucket_shared_latch);
  global_lock.unlock();
  return bucket->Find(key, value);
}

/*
 * delete <key,value> entry in hash table
 * Shrink & Combination is not required for this project
 */
template <typename K, typename V>
bool ExtendibleHash<K, V>::Remove(const K &key) {
  std::lock_guard<std::shared_mutex> global_lock(global_latch);
  size--;
  const auto bucket = GetBucket(key);
  return bucket->Remove(key);
}

/*
 * check the existence of a key
 * if exists, return true and UPDATE THE VALUE
 * if not exists, return false
 */
template <typename K, typename V>
bool ExtendibleHash<K, V>::Exists(const K &key, const V &value) {
  const auto bucket_j = GetBucket(key);
  std::lock_guard<std::shared_mutex> bucket_lock_guard(bucket_j->bucket_shared_latch);
  return bucket_j->Exist(key, value);
}

/*
 * insert <key,value> entry in hash table
 * Split & Redistribute bucket when there is overflow and if necessary increase
 * global depth
 */
template <typename K, typename V>
void ExtendibleHash<K, V>::Insert(const K &key, const V &value) {
  std::lock_guard<std::shared_mutex> global_lock(global_latch);
  // 0. Check the existence and update the value
  if (Exists(key, value)) return;

  // 1. Look up the correspond bucket
  check:
  auto bucket_j = GetBucket(key);
  std::lock_guard<std::shared_mutex> bucket_lock_guard(bucket_j->bucket_shared_latch);
  auto& keys_j = bucket_j->keys;
  auto& values_j = bucket_j->values;
  assert(keys_j.size() == values_j.size());
  assert(keys_j.size() <= bucketSize);

  // 2. Check the bucket, if is full
  if (keys_j.size() == bucketSize) {
    // 2.1 Bucket full
    const auto last_n_bits_global = GET_LAST_N_BITS(HashKey(key), globalDepth);
    const auto last_n_bits_local = GET_LAST_N_BITS(last_n_bits_global, bucket_j->localDepth);
    const auto old_localDepth = bucket_j->localDepth;

    // 2.1.1 Compare the global depth (i) and local depth (i_j)
    if (globalDepth == old_localDepth) {
      // 2.1.1.1 i == i_j, only one entry in the bucket address table points to bucket j

      // 2.1.1.1.1 increase the size of the bucket address table.
      //           It replaces each entry by two entries, both of which contain the same pointer as the original entry.
      globalDepth++;
      double_size_grow();
      assert((decltype(bucketAddressTable.size()))(1 << globalDepth) == bucketAddressTable.size());
    }

    // 2.1.1.2 i > i_j, more than one entry in the bucket address table points to bucket j.
    //                  The system can split bucket $j$ without increasing the size of the bucket address table
    //                  Do Nothing For This Case

    // 2.1.2 The system allocates a new bucket z,
    //         and sets i_j and i_z to the value that results from adding 1 to the original i_j value.
    bucket_j->localDepth++;
    auto bucket_z = std::make_shared<Bucket>(bucket_j->localDepth);
    numBuckets++;
    auto& keys_z = bucket_z->keys;
    auto& values_z = bucket_z->values;

    // 2.1.3 Next, to adjust the entries in the bucket address table that previously pointed to bucket j
    int base_point = (1 << old_localDepth) + last_n_bits_local;
    const int iter_incr = 1 << (bucket_j->localDepth);
    const size_t iterations = std::max(((globalDepth - bucket_j->localDepth) << 1), 1);
    for (size_t it = 0; it != iterations; it++, base_point += iter_incr) {
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

    // 2.1.5 Check if there is place to insert after rehashing
    const bool to_bucket_z = GET_LAST_N_TH_BIT(HashKey(key), bucket_j->localDepth);
    if((matches.empty() && !to_bucket_z) || matches.size() + to_bucket_z > bucketSize) {
      // 2.1.5.1 No place, so repeat again
      goto check;
    }

    // 2.1.5.2 Exists space, insert key and value
    if (to_bucket_z) {
      bucket_z->Insert(key, value);
    } else {
      bucket_j->Insert(key, value);
    }
  } else {
    // 2.2 Bucket not full, just simple insert
    bucket_j->Insert(key, value);
  }
  // Insert Done
  size++;
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
