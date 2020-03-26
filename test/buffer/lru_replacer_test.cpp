/**
 * lru_replacer_test.cpp
 */

#include <thread>
#include <cstdio>
#include <include/common/logger.h>

#include "buffer/lru_replacer.h"
#include "gtest/gtest.h"

namespace cmudb {

TEST(LRUReplacerTest, SampleTest) {
  LRUReplacer<int> lru_replacer;
  
  // push element into replacer
  lru_replacer.Insert(1);
  lru_replacer.Insert(2);
  lru_replacer.Insert(3);
  lru_replacer.Insert(4);
  lru_replacer.Insert(5);
  lru_replacer.Insert(6);
  lru_replacer.Insert(1);

  EXPECT_EQ(6, lru_replacer.Size());
  
  // pop element from replacer
  int value;
  lru_replacer.Victim(value);
  EXPECT_EQ(2, value);
  lru_replacer.Victim(value);
  EXPECT_EQ(3, value);
  lru_replacer.Victim(value);
  EXPECT_EQ(4, value);
  
  // remove element from replacer
  EXPECT_EQ(false, lru_replacer.Erase(4));
  EXPECT_EQ(true, lru_replacer.Erase(6));
  EXPECT_EQ(2, lru_replacer.Size());
  
  // pop element from replacer after removal
  lru_replacer.Victim(value);
  EXPECT_EQ(5, value);
  lru_replacer.Victim(value);
  EXPECT_EQ(1, value);
}

// ---------------------------------------------------------------------------------
// Test from https://github.com/yixuaz/CMU-15445/blob/master/cmu_15445_2017(sol).rar
// START FROM HERE
TEST(LRUReplacerTest, SampleTest1) {
    LRUReplacer<int> lru_replacer;
    int value;

    EXPECT_EQ(false, lru_replacer.Victim(value));

    lru_replacer.Insert(0);
    EXPECT_EQ(1, lru_replacer.Size());
    EXPECT_EQ(true, lru_replacer.Victim(value));
    EXPECT_EQ(0, value);
    EXPECT_EQ(false, lru_replacer.Victim(value));

    EXPECT_EQ(false, lru_replacer.Erase(0));
    EXPECT_EQ(0, lru_replacer.Size());

    lru_replacer.Insert(1);
    lru_replacer.Insert(1);
    lru_replacer.Insert(2);
    lru_replacer.Insert(2);
    lru_replacer.Insert(1);
    EXPECT_EQ(2, lru_replacer.Size());
    EXPECT_EQ(true, lru_replacer.Victim(value));
    EXPECT_EQ(2, value);

}

TEST(LRUReplacerTest, BasicTest) {
    LRUReplacer<int> lru_replacer;

    // push element into replacer
    for (int i = 0; i < 100; ++i) {
        lru_replacer.Insert(i);
    }
    EXPECT_EQ(100, lru_replacer.Size());

    // reverse then insert again
    for (int i = 0; i < 100; ++i) {
        lru_replacer.Insert(99 - i);
    }

    // erase 50 element from the tail
    for (int i = 0; i < 50; ++i) {
        EXPECT_EQ(true, lru_replacer.Erase(i));
    }

    // check left
    int value = -1;
    for (int i = 99; i >= 50; --i) {
        lru_replacer.Victim(value);
        EXPECT_EQ(i, value);
        value = -1;
    }
}
// Test from https://github.com/yixuaz/CMU-15445/blob/master/cmu_15445_2017(sol).rar
// END UNTIL HERE

// Added by Jigao
TEST(LRUReplacerTest, ConcurrentInsertTest) {
  const int num_runs = 500;
  const int num_threads = 20;
  // Run concurrent test multiple times to guarantee correctness.
  for (int run = 0; run < num_runs; run++) {
    LRUReplacer<int> test;
    std::vector<std::thread> threads;
    for (int tid = 0; tid < num_threads; tid++) {
      threads.push_back(std::thread([tid, &test]() {
        test.Insert(tid);
      }));
    }
    for (int i = 0; i < num_threads; i++) {
      threads[i].join();
    }
    EXPECT_EQ(test.Size(), num_threads);
    for (int i = 0; i < num_threads; i++) {
      EXPECT_TRUE(test.Erase(i));
      EXPECT_FALSE(test.Erase(i));
    }

    // No more element in LRU list
    int val;
    EXPECT_FALSE(test.Victim(val));
    EXPECT_EQ(test.Size(), 0);
  }
}

// Added by Jigao
TEST(LRUReplacerTest, ConcurrentMultiInsertTest) {
  const int num_runs = 500;
  const int num_threads = 20;
  const int num_insert_per_thread = 100;
  // Run concurrent test multiple times to guarantee correctness.
  for (int run = 0; run < num_runs; run++) {
    LRUReplacer<int> test;
    std::vector<std::thread> threads;
    for (int tid = 0; tid < num_threads; tid++) {
      threads.push_back(std::thread([tid, &test]() {
        // each thread inserts different values
        for (size_t i = 0; i < num_insert_per_thread; i++) test.Insert(tid * num_insert_per_thread + i);
      }));
    }
    for (int i = 0; i < num_threads; i++) {
      threads[i].join();
    }
    EXPECT_EQ(test.Size(), num_threads * num_insert_per_thread);
    for (int i = 0; i < num_threads * num_insert_per_thread; i++) {
      EXPECT_TRUE(test.Erase(i));
      EXPECT_FALSE(test.Erase(i));
    }

    // No more element in LRU list
    int val;
    EXPECT_FALSE(test.Victim(val));
    EXPECT_EQ(test.Size(), 0);
  }
}

// Added by Jigao
TEST(LRUReplacerTest, ConcurrentEraseTest) {
  const int num_runs = 500;
  const int num_threads = 20;
  const int num_insert_per_thread = 10;
  const int num_erase_per_thread = 5;

  // Run concurrent test multiple times to guarantee correctness.
  for (int run = 0; run < num_runs; run++) {
    LRUReplacer<int> test;
    std::vector<std::thread> threads;
    for (int tid = 0; tid < num_threads; tid++) {
      threads.push_back(std::thread([tid, &test]() {
        // each thread inserts different values
        for (size_t i = 0; i < num_insert_per_thread; i++) test.Insert(tid * num_insert_per_thread + i);
        // each thread erase part of values, which inserted by this thread
        for (size_t i = 0; i < num_erase_per_thread; i++) test.Erase(tid * num_insert_per_thread + i);
      }));
    }
    for (int i = 0; i < num_threads; i++) {
      threads[i].join();
    }
    EXPECT_EQ(test.Size(), num_threads * (num_insert_per_thread - num_erase_per_thread));
    threads.clear();

    // Multitheading Erase: which inserted or already erased by this thread
    for (int tid = 0; tid < num_threads; tid++) {
      threads.push_back(std::thread([tid, &test]() {
        for (int erase_i = 0; erase_i < num_erase_per_thread; erase_i++) {
          EXPECT_FALSE(test.Erase(tid * num_insert_per_thread + erase_i));
        }
        for (int insert_i = 0; insert_i < num_insert_per_thread - num_erase_per_thread; insert_i++) {
          EXPECT_TRUE(test.Erase(tid * num_insert_per_thread + num_erase_per_thread + insert_i));
          EXPECT_FALSE(test.Erase(tid * num_insert_per_thread + num_erase_per_thread + insert_i));
        }
      }));
    }
    for (int i = 0; i < num_threads; i++) {
      threads[i].join();
    }

    // No more element in LRU list
    int val;
    EXPECT_FALSE(test.Victim(val));
    EXPECT_EQ(test.Size(), 0);
  }
}

// Added by Jigao
TEST(LRUReplacerTest, ConcurrentVictimTest) {
  const int num_runs = 500;
  const int num_threads = 20;
  const int num_insert_per_thread = 10;
  const int num_victim_per_thread = 5;

  // Run concurrent test multiple times to guarantee correctness.
  for (int run = 0; run < num_runs; run++) {
    LRUReplacer<int> test;
    std::vector<std::thread> threads;
    for (int tid = 0; tid < num_threads; tid++) {
      threads.push_back(std::thread([tid, &test]() {
        // each thread inserts different values
        for (size_t i = 0; i < num_insert_per_thread; i++) test.Insert(tid * num_insert_per_thread + i);
        // each thread victimize part of values, which inserted by this thread
        int value;
        for (size_t i = 0; i < num_victim_per_thread; i++) test.Victim(value);
      }));
    }
    for (int i = 0; i < num_threads; i++) {
      threads[i].join();
    }
    EXPECT_EQ(test.Size(), num_threads * (num_insert_per_thread - num_victim_per_thread));
    threads.clear();

    // Multitheading Victim: which inserted or already victimized by this thread
    for (int tid = 0; tid < num_threads; tid++) {
      threads.push_back(std::thread([tid, &test]() {
        for (int i = 0; i < (num_insert_per_thread - num_victim_per_thread); i++) {
          int value;
          test.Victim(value);
        }
      }));
    }
    for (int i = 0; i < num_threads; i++) {
      threads[i].join();
    }

    // No more element in LRU list
    int val;
    EXPECT_FALSE(test.Victim(val));
    EXPECT_EQ(test.Size(), 0);
  }
}

// ---------------------------------------------------------------------------------

} // namespace cmudb
