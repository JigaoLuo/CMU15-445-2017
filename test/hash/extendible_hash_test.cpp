/**
 * extendible_hash_test.cpp
 */

#include <thread>
#include <random>
#include <algorithm>
#include <unordered_map>
#include <map>


#include "hash/extendible_hash.h"
#include "gtest/gtest.h"

namespace cmudb {

TEST(ExtendibleHashTest, SampleTest) {
  // set leaf size as 2
  ExtendibleHash<int, std::string> *test =
      new ExtendibleHash<int, std::string>(2);

  // insert several key/value pairs
  test->Insert(1, "a");
  test->Insert(2, "b");
  test->Insert(3, "c");
  test->Insert(4, "d");
  test->Insert(5, "e");
  test->Insert(6, "f");
  test->Insert(7, "g");
  test->Insert(8, "h");
  test->Insert(9, "i");
  EXPECT_EQ(2, test->GetLocalDepth(0));
  EXPECT_EQ(3, test->GetLocalDepth(1));
  EXPECT_EQ(2, test->GetLocalDepth(2));
  EXPECT_EQ(2, test->GetLocalDepth(3));
  EXPECT_EQ(3, test->GetLocalDepth(5));  // Added by Jigao Luo

  // find test
  std::string result;
  test->Find(9, result);
  EXPECT_EQ("i", result);
  test->Find(8, result);
  EXPECT_EQ("h", result);
  test->Find(2, result);
  EXPECT_EQ("b", result);
  EXPECT_EQ(0, test->Find(10, result));

  // delete test
  EXPECT_EQ(1, test->Remove(8));
  EXPECT_EQ(0, test->Find(8, result));  // Added by Jigao Luo
  EXPECT_EQ(1, test->Remove(4));
  EXPECT_EQ(0, test->Find(4, result));  // Added by Jigao Luo
  EXPECT_EQ(1, test->Remove(1));
  EXPECT_EQ(0, test->Find(1, result));  // Added by Jigao Luo
  EXPECT_EQ(0, test->Remove(20));

  delete test;
}

// ---------------------------------------------------------------------------------
// Test from https://github.com/yixuaz/CMU-15445/blob/master/cmu_15445_2017(sol).rar
//           https://github.com/liu-jianhao/CMU-15-445/tree/master/Lab/test
// START FROM HERE
TEST(ExtendibleHashTest, SampleTest2) {
  // set leaf size as 2
  ExtendibleHash<int, std::string> *test =
      new ExtendibleHash<int, std::string>(2);

  // insert several key/value pairs
  test->Insert(1, "a");
  test->Insert(2, "b");
  test->Insert(3, "c");
  test->Insert(4, "d");
  test->Insert(5, "e");
  test->Insert(6, "f");
  test->Insert(7, "g");
  test->Insert(8, "h");
  test->Insert(9, "i");
  EXPECT_EQ(2, test->GetLocalDepth(0));
  EXPECT_EQ(3, test->GetLocalDepth(1));
  EXPECT_EQ(2, test->GetLocalDepth(2));
  EXPECT_EQ(2, test->GetLocalDepth(3));
  EXPECT_EQ(3, test->GetLocalDepth(5));  // Added by Jigao Luo

  // find test
  std::string result;
  test->Find(9, result);
  EXPECT_EQ("i", result);
  test->Find(8, result);
  EXPECT_EQ("h", result);
  test->Find(2, result);
  EXPECT_EQ("b", result);
  EXPECT_EQ(0, test->Find(10, result));

  // delete test
  EXPECT_EQ(1, test->Remove(8));
  EXPECT_EQ(0, test->Find(8, result));  // Added by Jigao Luo
  EXPECT_EQ(1, test->Remove(4));
  EXPECT_EQ(0, test->Find(4, result));  // Added by Jigao Luo
  EXPECT_EQ(1, test->Remove(1));
  EXPECT_EQ(0, test->Find(1, result));  // Added by Jigao Luo
  EXPECT_EQ(0, test->Remove(20));

  test->Insert(1, "a");
  test->Insert(2, "b");
  test->Insert(3, "c");
  test->Insert(4, "d");
  test->Insert(5, "e");
  test->Insert(6, "f");
  test->Insert(7, "g");
  test->Insert(8, "h");
  test->Insert(9, "i");

  test->Find(9, result);
  EXPECT_EQ("i", result);
  test->Find(8, result);
  EXPECT_EQ("h", result);
  test->Find(2, result);
  EXPECT_EQ("b", result);
  EXPECT_EQ(0, test->Find(10, result));
  delete test;
}

// first split increase global depth from 0 to 3
TEST(ExtendibleHashTest, BasicDepthTest) {
  // set leaf size as 2
  ExtendibleHash<int, std::string> *test =
      new ExtendibleHash<int, std::string>(2);

  // insert several key/value pairs
  test->Insert(6, "a");   // b'0110
  test->Insert(10, "b");  // b'1010
  test->Insert(14, "c");  // b'1110

  EXPECT_EQ(3, test->GetGlobalDepth());

  EXPECT_EQ(3, test->GetLocalDepth(2));
  EXPECT_EQ(3, test->GetLocalDepth(6));

  EXPECT_EQ(2, test->GetLocalDepth(0));  // Modified by Jigao Luo
  EXPECT_EQ(2, test->GetLocalDepth(4));  // Modified by Jigao Luo

  EXPECT_EQ(1, test->GetLocalDepth(1));  // Modified by Jigao Luo
  EXPECT_EQ(1, test->GetLocalDepth(3));  // Modified by Jigao Luo
  EXPECT_EQ(1, test->GetLocalDepth(5));  // Modified by Jigao Luo
  EXPECT_EQ(1, test->GetLocalDepth(7));  // Modified by Jigao Luo

  // four buckets in use
  EXPECT_EQ(4, test->GetNumBuckets());

  // insert more key/value pairs
  test->Insert(1, "d");
  test->Insert(3, "e");
  test->Insert(5, "f");

  EXPECT_EQ(5, test->GetNumBuckets());

  EXPECT_EQ(3, test->GetGlobalDepth());  // Added by Jigao Luo

  EXPECT_EQ(2, test->GetLocalDepth(0));  // Added by Jigao Luo

  EXPECT_EQ(2, test->GetLocalDepth(1));
  EXPECT_EQ(2, test->GetLocalDepth(5));

  EXPECT_EQ(3, test->GetLocalDepth(2));  // Added by Jigao Luo

  EXPECT_EQ(2, test->GetLocalDepth(3));
  EXPECT_EQ(2, test->GetLocalDepth(7));  // Added by Jigao Luo

  EXPECT_EQ(3, test->GetLocalDepth(6));  // Added by Jigao Luo
  delete test;
}


TEST(ExtendibleHashTest, BasicSeqTest) {
  ExtendibleHash<int, int> *test = new ExtendibleHash<int, int>(100);

  // insert
  std::unordered_map <int, int> comparator;
  for (int i = 0; i < 1000000; ++i) {
    auto item = i;
    comparator.emplace(item,item);
    test->Insert(item, item);
  }

  // compare result
  int value = 0;
  for (const auto& i: comparator) {
    test->Find(i.first, value);
    EXPECT_EQ(i.first, value);
    // delete
    EXPECT_EQ(1, test->Remove(value));
    // find again will fail
    value = 0;
    EXPECT_EQ(0, test->Find(i.first, value));
  }


  delete test;
}

TEST(ExtendibleHashTest, BasicRandomTest) {
  ExtendibleHash<int, int> *test = new ExtendibleHash<int, int>(100);

  // insert
  int seed = time(nullptr);
  std::cerr << "seed: " << seed << std::endl;
  std::default_random_engine engine(seed);
  std::uniform_int_distribution<int> distribution(0, 1000000);
  std::map<int, int> comparator;

  for (int i = 0; i < 1000000; ++i) {
      auto item = distribution(engine);
    comparator.emplace(item,item);
    test->Insert(item, item);
  }

  // compare result
  int value = 0;
  for (const auto& i: comparator) {
    test->Find(i.first, value);
    EXPECT_EQ(i.first, value);
    // delete
    EXPECT_EQ(1, test->Remove(value));
    // find again will fail
    value = 0;
    EXPECT_EQ(0, test->Find(i.first, value));
  }

  delete test;
}

TEST(ExtendibleHashTest, LargeRandomInsertTest) {
  ExtendibleHash<int, int> *test =
      new ExtendibleHash<int, int>(100);

  int counter = 0;
  for (size_t i = 0; i< 1000000; i++) {
    srand(time(0) + i);
    if (random() % 3) {
      // Insert [0, counter]
      test->Insert(counter, counter);
      counter++;
    } else {
      if (counter > 0) {
        // Find [0, counter], which are already inserted
        int value = 0;
        const int x = random() % counter;
        EXPECT_EQ(true, test->Find(x, value));
        EXPECT_EQ(x, value);
      }
    }
  }

  delete test;
}

TEST(ExtendibleHashTest, RandomInsertAndDeleteTest) {
  // set leaf size as 2
  ExtendibleHash<int, int> *test =
      new ExtendibleHash<int, int>(100);

  for (int i = 0; i < 1000000; i++) {
    test->Insert(i, i);
  }

  for (int i = 0; i < 1000000; i++) {
    srand(time(0) + i);
    if (rand()% 2 == 0) {
      test->Remove(i);
      int value;
      EXPECT_NE(test->Find(i, value), true);
    } else {
      test->Insert(i, i + 2);
      int value;
      EXPECT_EQ(test->Find(i, value), true);
      EXPECT_EQ(value, i + 2);
    }
  }

  delete test;
}
// Test from https://github.com/yixuaz/CMU-15445/blob/master/cmu_15445_2017(sol).rar
//           https://github.com/liu-jianhao/CMU-15-445/tree/master/Lab/test
// END UNTIL HERE
// ---------------------------------------------------------------------------------

TEST(ExtendibleHashTest, ConcurrentInsertTest) {
  const int num_runs = 500;
  const int num_threads = 3;
  // Run concurrent test multiple times to guarantee correctness.
  for (int run = 0; run < num_runs; run++) {
    std::shared_ptr<ExtendibleHash<int, int>> test{new ExtendibleHash<int, int>(2)};
    std::vector<std::thread> threads;
    for (int tid = 0; tid < num_threads; tid++) {
      threads.push_back(std::thread([tid, &test]() {
        test->Insert(tid, tid);
      }));
    }
    for (int i = 0; i < num_threads; i++) {
      threads[i].join();
    }
    EXPECT_EQ(test->GetGlobalDepth(), 1);
    for (int i = 0; i < num_threads; i++) {
      int val;
      EXPECT_TRUE(test->Find(i, val));
      EXPECT_EQ(val, i);
    }
  }
}

TEST(ExtendibleHashTest, ConcurrentRemoveTest) {
  const int num_threads = 5;
  const int num_runs = 500;
  for (int run = 0; run < num_runs; run++) {
    std::shared_ptr<ExtendibleHash<int, int>> test{new ExtendibleHash<int, int>(2)};
    std::vector<std::thread> threads;
    std::vector<int> values{0, 10, 16, 32, 64};
    for (int value : values) {
      test->Insert(value, value);
    }
    EXPECT_EQ(test->GetGlobalDepth(), 6);
    for (int tid = 0; tid < num_threads; tid++) {
      threads.push_back(std::thread([tid, &test, &values]() {
        test->Remove(values[tid]);
        test->Insert(tid + 4, tid + 4);
      }));
    }
    for (int i = 0; i < num_threads; i++) {
      threads[i].join();
    }
    EXPECT_EQ(test->GetGlobalDepth(), 6);
    int val;
    EXPECT_EQ(0, test->Find(0, val));
    EXPECT_EQ(1, test->Find(8, val));
    EXPECT_EQ(0, test->Find(16, val));
    EXPECT_EQ(0, test->Find(3, val));
    EXPECT_EQ(1, test->Find(4, val));
  }
}

// Added by Jigao
TEST(ExtendibleHashTest, EnormeConcurrentInsertTest) {
  const int num_runs = 50;
  const int num_threads = 20;
  const int num_per_threads = 1000;
  std::array<std::vector<int>, num_threads> vectors;
  int counter = 0;
  for (auto& vec : vectors) {
    vec.reserve(num_per_threads);
    for (int i = 0; i < num_per_threads; i++) vec.emplace_back(counter++);
  }

  // Run concurrent test multiple times to guarantee correctness.
  for (int run = 0; run < num_runs; run++) {
    std::shared_ptr<ExtendibleHash<int, int>> test{new ExtendibleHash<int, int>(100)};
    std::vector<std::thread> threads;
    for (int tid = 0; tid < num_threads; tid++) {
      threads.push_back(std::thread([tid, &test, &vectors]() {
        for (const auto ele : vectors[tid]) test->Insert(ele, ele);
      }));
    }
    for (int i = 0; i < num_threads; i++) {
      threads[i].join();
    }

    for (const auto& vec : vectors) {
      int val;
      for (const auto ele : vec) {
        EXPECT_TRUE(test->Find(ele, val));
        EXPECT_EQ(val, ele);
      }
    }
  }
}

// Added by Jigao
TEST(ExtendibleHashTest, EnormeRandomConcurrentInsertTest) {
  const int num_runs = 50;
  const int num_threads = 20;
  const int num_per_threads = 1000;
  std::array<std::vector<int>, num_threads> vectors;

  int seed = time(nullptr);
  std::cerr << "seed: " << seed << std::endl;
  std::default_random_engine engine(seed);
  std::uniform_int_distribution<int> distribution(0, 1000000);

  for (auto& vec : vectors) {
    vec.reserve(num_per_threads);
    for (int i = 0; i < num_per_threads; i++) vec.emplace_back(distribution(engine));
  }

  // Run concurrent test multiple times to guarantee correctness.
  for (int run = 0; run < num_runs; run++) {
    std::shared_ptr<ExtendibleHash<int, int>> test{new ExtendibleHash<int, int>(100)};
    std::vector<std::thread> threads;
    for (int tid = 0; tid < num_threads; tid++) {
      threads.push_back(std::thread([tid, &test, &vectors]() {
        for (const auto ele : vectors[tid]) test->Insert(ele, ele);
      }));
    }
    for (int i = 0; i < num_threads; i++) {
      threads[i].join();
    }

    for (const auto& vec : vectors) {
      int val;
      for (const auto ele : vec) {
        EXPECT_TRUE(test->Find(ele, val));
        EXPECT_EQ(val, ele);
      }
    }
  }
}

// Added by Jigao
TEST(ExtendibleHashTest, EnormeConcurrentRemoveTest) {
  const int num_runs = 500;
  const int num_threads = 20;
  const int num_per_threads = 1000;
  std::array<std::vector<int>, num_threads> vectors;
  int counter = 0;
  for (auto& vec : vectors) {
    vec.reserve(num_per_threads);
    for (int i = 0; i < num_per_threads; i++) vec.emplace_back(counter++);
  }


  // Run concurrent test multiple times to guarantee correctness.
  for (int run = 0; run < num_runs; run++) {
    std::shared_ptr<ExtendibleHash<int, int>> test{new ExtendibleHash<int, int>(100)};
    for (const auto& vec : vectors) {
      for (const auto ele : vec) {
        test->Insert(ele, ele);
      }
    }

    std::vector<std::thread> threads;
    for (int tid = 0; tid < num_threads; tid++) {
      threads.push_back(std::thread([tid, &test, &vectors]() {
        for (const auto ele : vectors[tid]) {
          EXPECT_TRUE(test->Remove(ele));
          EXPECT_FALSE(test->Remove(ele));
        }
      }));
    }
    for (int i = 0; i < num_threads; i++) {
      threads[i].join();
    }
  }
}

// Added by Jigao
TEST(ExtendibleHashTest, EnormeRandomConcurrentRemoveTest) {
  const int num_runs = 500;
  const int num_threads = 20;
  const int num_per_threads = 1000;
  std::array<std::set<int>, num_threads> sets;

  int seed = time(nullptr);
  std::cerr << "seed: " << seed << std::endl;
  std::default_random_engine engine(seed);

  int it = 0;
  for (auto& set : sets) {
    std::uniform_int_distribution<int> distribution(100000 * it, 100000 * (it + 1));
    for (int i = 0; i < num_per_threads; i++) set.emplace(distribution(engine));
    it++;
  }

  // Run concurrent test multiple times to guarantee correctness.
  for (int run = 0; run < num_runs; run++) {
    std::shared_ptr<ExtendibleHash<int, int>> test{new ExtendibleHash<int, int>(100)};
    for (const auto& set : sets) {
      for (const auto ele : set) {
        test->Insert(ele, ele);
      }
    }

    std::vector<std::thread> threads;
    for (int tid = 0; tid < num_threads; tid++) {
      threads.push_back(std::thread([tid, &test, &sets]() {
        for (const auto ele : sets[tid]) {
          EXPECT_TRUE(test->Remove(ele));
          EXPECT_FALSE(test->Remove(ele));
        }
      }));
    }
    for (int i = 0; i < num_threads; i++) {
      threads[i].join();
    }
  }
}

// Added by Jigao
TEST(ExtendibleHashTest, EnormeConcurrentTest) {
  const int num_runs = 500;
  const int num_threads = 20;
  const int num_per_threads = 1000;
  std::array<std::set<int>, num_threads> sets;

  int seed = time(nullptr);
  std::cerr << "seed: " << seed << std::endl;
  std::default_random_engine engine(seed);

  int it = 0;
  for (auto& set : sets) {
    std::uniform_int_distribution<int> distribution(100000 * it, 100000 * (it + 1) - 1);
    for (int i = 0; i < num_per_threads; i++) set.emplace(distribution(engine));
    it++;
  }


  // Run concurrent test multiple times to guarantee correctness.
  for (int run = 0; run < num_runs; run++) {
    std::shared_ptr<ExtendibleHash<int, int>> test{new ExtendibleHash<int, int>(100)};

    for (decltype(sets.size()) i = 0; i < sets.size(); ++i) {
      if (i % 3 != 0) {
        for (const auto ele : sets[i])  test->Insert(ele, ele);
      }
    }

    std::vector<std::thread> threads;
    for (int tid = 0; tid < num_threads; tid++) {
      if (tid % 3 == 0) {
        // Insert
        threads.push_back(std::thread([tid, &test, &sets]() {
          for (const auto ele : sets[tid]) test->Insert(ele, ele);
        }));
      } else if (tid % 3 == 1) {
        // Find
        threads.push_back(std::thread([tid, &test, &sets]() {
          for (const auto ele : sets[tid]) {
            int val = 0;
            EXPECT_TRUE(test->Find(ele, val));
            EXPECT_EQ(val, ele);
          }
        }));
      } else {
        // Remove
        threads.push_back(std::thread([tid, &test, &sets]() {
          for (const auto ele : sets[tid]) {
            EXPECT_TRUE(test->Remove(ele));
            EXPECT_FALSE(test->Remove(ele));
            int val = 0;
            EXPECT_FALSE(test->Find(ele, val));
          }
        }));
      }
    }
    for (int i = 0; i < num_threads; i++) {
      threads[i].join();
    }
  }
}
//
TEST(ExtendibleHashTest, EnormeRandomConcurrentTest) {
  const int num_runs = 500;
  const int num_threads = 20;
  const int num_per_threads = 1000;
  std::array<std::vector<int>, num_threads> vectors;
  int counter = 0;
  for (auto& vec : vectors) {
    vec.reserve(num_per_threads);
    for (int i = 0; i < num_per_threads; i++) vec.emplace_back(counter++);
  }


  // Run concurrent test multiple times to guarantee correctness.
  for (int run = 0; run < num_runs; run++) {
    std::shared_ptr<ExtendibleHash<int, int>> test{new ExtendibleHash<int, int>(100)};

    for (decltype(vectors.size()) i = 0; i < vectors.size(); ++i) {
      if (i % 3 != 0) {
        for (const auto ele : vectors[i])  test->Insert(ele, ele);
      }
    }

    std::vector<std::thread> threads;
    for (int tid = 0; tid < num_threads; tid++) {
      if (tid % 3 == 0) {
        // Insert
        threads.push_back(std::thread([tid, &test, &vectors]() {
          for (const auto ele : vectors[tid]) test->Insert(ele, ele);
        }));
      } else if (tid % 3 == 1) {
        // Find
        threads.push_back(std::thread([tid, &test, &vectors]() {
          for (const auto ele : vectors[tid]) {
            int val = 0;
            EXPECT_TRUE(test->Find(ele, val));
            EXPECT_EQ(val, ele);
          }
        }));
      } else {
        // Remove
        threads.push_back(std::thread([tid, &test, &vectors]() {
          for (const auto ele : vectors[tid]) {
            EXPECT_TRUE(test->Remove(ele));
            EXPECT_FALSE(test->Remove(ele));
            int val = 0;
            EXPECT_FALSE(test->Find(ele, val));
          }
        }));
      }
    }
    for (int i = 0; i < num_threads; i++) {
      threads[i].join();
    }
  }
}

TEST(ExtendibleHashTest, EnormeRandomConcurrentTest2) {
  const int num_runs = 500;
  const int num_threads = 20;
  const int num_per_threads = 1000;
  std::array<std::vector<int>, num_threads> vectors;
  int counter = 0;
  for (auto& vec : vectors) {
    vec.reserve(num_per_threads);
    for (int i = 0; i < num_per_threads; i++) vec.emplace_back(counter++);
  }


  // Run concurrent test multiple times to guarantee correctness.
  for (int run = 0; run < num_runs; run++) {
    std::shared_ptr<ExtendibleHash<int, int>> test{new ExtendibleHash<int, int>(100)};

    std::vector<std::thread> threads;
    for (int tid = 0; tid < num_threads; tid++) {
      if (tid % 3 == 0) {
        // Insert
        threads.push_back(std::thread([tid, &test, &vectors]() {
          for (const auto ele : vectors[tid]) test->Insert(ele, ele);
        }));
      } else if (tid % 3 == 1) {
        // Find
        threads.push_back(std::thread([tid, &test, &vectors]() {
          for (const auto ele : vectors[tid]) {
            int val = 0;
            test->Find(ele, val);
          }
        }));
      } else {
        // Remove
        threads.push_back(std::thread([tid, &test, &vectors]() {
          for (const auto ele : vectors[tid]) {
            test->Remove(ele);
            int val = 0;
            EXPECT_FALSE(test->Find(ele, val));
          }
        }));
      }
    }
    for (int i = 0; i < num_threads; i++) {
      threads[i].join();
    }
  }
}

} // namespace cmudb
