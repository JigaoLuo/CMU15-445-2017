/**
 * buffer_pool_manager_test.cpp
 */

#include <cstdio>

#include "buffer/buffer_pool_manager.h"
#include "gtest/gtest.h"

namespace cmudb {
//TODO: add more multithread tests
TEST(BufferPoolManagerTest, SampleTest) {
  page_id_t temp_page_id;

  DiskManager *disk_manager = new DiskManager("test.db");
  BufferPoolManager bpm(10, disk_manager);

  auto page_zero = bpm.NewPage(temp_page_id);
  EXPECT_NE(nullptr, page_zero);
  EXPECT_EQ(0, temp_page_id);
  EXPECT_EQ(1, page_zero->GetPinCount());  // Added by Jigao
  EXPECT_EQ(1, bpm.GetPageTableSize());  // Added by Jigao, GetPageTableSize is a function for test

  // The test will fail here if the page is null
  ASSERT_NE(nullptr, page_zero);

  // change content in page one
  strcpy(page_zero->GetData(), "Hello");

  for (int i = 1; i < 10; ++i) {
    EXPECT_NE(nullptr, bpm.NewPage(temp_page_id));
    EXPECT_EQ(i, temp_page_id);  // Added by Jigao
    EXPECT_EQ(1, bpm.GetPagePinCount(i));  // Added by Jigao, GetPagePinCount is a function for test
    EXPECT_EQ(i + 1, bpm.GetPageTableSize());  // Added by Jigao, GetPageTableSize is a function for test
  }
  // all the pages are pinned, the buffer pool is full
  for (int i = 10; i < 15; ++i) {
    EXPECT_EQ(nullptr, bpm.NewPage(temp_page_id));
    EXPECT_EQ(INVALID_PAGE_ID, temp_page_id);  // Added by Jigao
    EXPECT_EQ(10, bpm.GetPageTableSize());  // Added by Jigao, GetPageTableSize is a function for test
  }
  // upin the first five pages, add them to LRU list, set as dirty
  for (int i = 0; i < 5; ++i) {
    EXPECT_EQ(true, bpm.UnpinPage(i, true));
    EXPECT_EQ(0, bpm.GetPagePinCount(i));  // Added by Jigao, GetPagePinCount is a function for test
    EXPECT_EQ(i + 1, bpm.GetReplacerSize());  // Added by Jigao, GetReplacerSize is a function for test
    EXPECT_EQ(10, bpm.GetPageTableSize());  // Added by Jigao, GetPageTableSize is a function for test
  }
  // we have 5 empty slots in LRU list, evict page zero out of buffer pool
  for (int i = 10, replacer_size = 4; i < 14; ++i, --replacer_size) {  // Modified by Jigao
    EXPECT_NE(nullptr, bpm.NewPage(temp_page_id));
    EXPECT_EQ(i, temp_page_id);  // Added by Jigao
    EXPECT_EQ(replacer_size, bpm.GetReplacerSize());  // Added by Jigao, GetReplacerSize is a function for test
    EXPECT_EQ(10, bpm.GetPageTableSize());  // Added by Jigao, GetPageTableSize is a function for test
  }
  EXPECT_EQ(1, bpm.GetReplacerSize());  // Added by Jigao, GetReplacerSize is a function for test

  // fetch page one again
  EXPECT_FALSE(bpm.FindInBuffer(0));  // Added by Jigao, FindInBuffer is a function for test
  page_zero = bpm.FetchPage(0);
  EXPECT_TRUE(bpm.FindInBuffer(0));  // Added by Jigao, FindInBuffer is a function for test
  EXPECT_EQ(0, bpm.GetReplacerSize());  // Added by Jigao, GetReplacerSize is a function for test
  EXPECT_EQ(10, bpm.GetPageTableSize());  // Added by Jigao, GetPageTableSize is a function for test
  EXPECT_EQ(1, bpm.GetPagePinCount(0));  // Added by Jigao, GetPagePinCount is a function for test

  // check read content
  EXPECT_EQ(0, strcmp(page_zero->GetData(), "Hello"));

  remove("test.db");
}

// ---------------------------------------------------------------------------------
// Test from https://github.com/yixuaz/CMU-15445/blob/master/cmu_15445_2017(sol).rar
// START FROM HERE
TEST(BufferPoolManagerTest, SampleTest2) {
  page_id_t temp_page_id;

  DiskManager *disk_manager = new DiskManager("test.db");
  BufferPoolManager bpm(10, disk_manager);

  auto page_zero = bpm.NewPage(temp_page_id);
  EXPECT_NE(nullptr, page_zero);
  EXPECT_EQ(0, temp_page_id);
  EXPECT_EQ(1, page_zero->GetPinCount());  // Added by Jigao
  EXPECT_EQ(1, bpm.GetPageTableSize());  // Added by Jigao, GetPageTableSize is a function for test

  // The test will fail here if the page is null
  ASSERT_NE(nullptr, page_zero);

  // change content in page one
  strcpy(page_zero->GetData(), "Hello");

  for (int i = 1; i < 10; ++i) {
    EXPECT_NE(nullptr, bpm.NewPage(temp_page_id));
    EXPECT_EQ(i, temp_page_id);  // Added by Jigao
    EXPECT_EQ(1, bpm.GetPagePinCount(i));  // Added by Jigao, GetPagePinCount is a function for test
    EXPECT_EQ(i + 1, bpm.GetPageTableSize());  // Added by Jigao, GetPageTableSize is a function for test
  }

  // all the pages are pinned, the buffer pool is full
  for (int i = 10; i < 15; ++i) {
    EXPECT_EQ(nullptr, bpm.NewPage(temp_page_id));
    EXPECT_EQ(INVALID_PAGE_ID, temp_page_id);  // Added by Jigao
    EXPECT_EQ(10, bpm.GetPageTableSize());  // Added by Jigao, GetPageTableSize is a function for test
  }

  // upin the first page, add them to LRU list, set as dirty
  for (int i = 0; i < 1; ++i) {
    EXPECT_TRUE(bpm.UnpinPage(i, true));
    EXPECT_EQ(0, bpm.GetPagePinCount(i));  // Added by Jigao, GetPagePinCount is a function for test
    EXPECT_EQ(1, bpm.GetReplacerSize());  // Added by Jigao, GetReplacerSize is a function for test
    EXPECT_EQ(10, bpm.GetPageTableSize());  // Added by Jigao, GetPageTableSize is a function for test

    EXPECT_TRUE(bpm.FindInBuffer(0));  // Added by Jigao, FindInBuffer is a function for test
    page_zero = bpm.FetchPage(0);
    EXPECT_TRUE(bpm.FindInBuffer(0));  // Added by Jigao, FindInBuffer is a function for test
    EXPECT_EQ(0, bpm.GetReplacerSize());  // Added by Jigao, GetReplacerSize is a function for test
    EXPECT_EQ(10, bpm.GetPageTableSize());  // Added by Jigao, GetPageTableSize is a function for test
    EXPECT_EQ(1, bpm.GetPagePinCount(i));  // Added by Jigao, GetPagePinCount is a function for test
    EXPECT_EQ(0, strcmp(page_zero->GetData(), "Hello"));

    EXPECT_TRUE(bpm.UnpinPage(i, true));
    EXPECT_EQ(0, bpm.GetPagePinCount(i));  // Added by Jigao, GetPagePinCount is a function for test
    EXPECT_EQ(1, bpm.GetReplacerSize());  // Added by Jigao, GetReplacerSize is a function for test
    EXPECT_EQ(10, bpm.GetPageTableSize());  // Added by Jigao, GetPageTableSize is a function for test

    EXPECT_TRUE(bpm.FindInBuffer(0));  // Added by Jigao, FindInBuffer is a function for test
    EXPECT_NE(nullptr, bpm.NewPage(temp_page_id));
    EXPECT_FALSE(bpm.FindInBuffer(0));  // Added by Jigao, FindInBuffer is a function for test
    EXPECT_EQ(10, temp_page_id);
    EXPECT_EQ(1, bpm.GetPagePinCount(10));  // Added by Jigao, GetPagePinCount is a function for test
    EXPECT_EQ(0, bpm.GetReplacerSize());  // Added by Jigao, GetReplacerSize is a function for test
  }

  std::vector<int> test{5, 6, 7, 8, 9, 10};

  for (auto v: test) {
    EXPECT_EQ(1, bpm.GetPagePinCount(v));  // Added by Jigao, GetPagePinCount is a function for test
    Page* page = bpm.FetchPage(v);
    EXPECT_EQ(2, bpm.GetPagePinCount(v));  // Added by Jigao, GetPagePinCount is a function for test
    EXPECT_NE(page, nullptr);
    EXPECT_EQ(v, page->GetPageId());
    bpm.UnpinPage(v, true);
    EXPECT_EQ(1, bpm.GetPagePinCount(v));  // Added by Jigao, GetPagePinCount is a function for test
    EXPECT_EQ(10, bpm.GetPageTableSize());  // Added by Jigao, GetPageTableSize is a function for test
  }

  EXPECT_EQ(0, bpm.GetReplacerSize());  // Added by Jigao, GetReplacerSize is a function for test
  bpm.UnpinPage(10, true);
  EXPECT_EQ(0, bpm.GetPagePinCount(10));  // Added by Jigao, GetPagePinCount is a function for test
  EXPECT_EQ(1, bpm.GetReplacerSize());  // Added by Jigao, GetReplacerSize is a function for test

  // fetch page one again
  EXPECT_FALSE(bpm.FindInBuffer(0));  // Added by Jigao, FindInBuffer is a function for test
  page_zero = bpm.FetchPage(0);
  EXPECT_TRUE(bpm.FindInBuffer(0));  // Added by Jigao, FindInBuffer is a function for test
  EXPECT_EQ(0, bpm.GetReplacerSize());  // Added by Jigao, GetReplacerSize is a function for test
  EXPECT_EQ(10, bpm.GetPageTableSize());  // Added by Jigao, GetPageTableSize is a function for test
  EXPECT_EQ(1, bpm.GetPagePinCount(0));  // Added by Jigao, GetPagePinCount is a function for test

  // check read content
  EXPECT_EQ(0, strcmp(page_zero->GetData(), "Hello"));

  remove("test.db");
}
// Test from https://github.com/yixuaz/CMU-15445/blob/master/cmu_15445_2017(sol).rar
// END UNTIL HERE
// ---------------------------------------------------------------------------------

} // namespace cmudb
