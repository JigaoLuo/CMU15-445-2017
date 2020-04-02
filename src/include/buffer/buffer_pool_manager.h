/*
 * buffer_pool_manager.h
 *
 * Functionality: The simplified Buffer Manager interface allows a client to
 * new/delete pages on disk, to read a disk page into the buffer pool and pin
 * it, also to unpin a page in the buffer pool.
 */

#pragma once
#include <list>
#include <shared_mutex>

#include "buffer/lru_replacer.h"
#include "disk/disk_manager.h"
#include "hash/extendible_hash.h"
#include "logging/log_manager.h"
#include "page/page.h"

namespace cmudb {
class BufferPoolManager {
public:
  BufferPoolManager(size_t pool_size, DiskManager *disk_manager,
                          LogManager *log_manager = nullptr);

  ~BufferPoolManager();

  Page *FetchPage(page_id_t page_id);

  bool UnpinPage(page_id_t page_id, bool is_dirty);

  bool FlushPage(page_id_t page_id);

  void FlushAllPages();

  Page *NewPage(page_id_t &page_id);

  bool DeletePage(page_id_t page_id);

  /**
   * Get pin count of a page id (Added by Jigao for test case)
   * @param page_id
   * @return the pin count of the page id
   */
  inline int GetPagePinCount(page_id_t page_id) {
    Page *page = nullptr;
    std::shared_lock<std::shared_mutex> lock(latch_);
    page_table_->Find(page_id, page);
    assert(page != nullptr);
    assert(FindInBuffer(page_id));
    return page->GetPinCount();
  }

  /**
   * Get the size of replacer (Added by Jigao for test case)
   * @return the size of replacer
   */
  inline size_t GetReplacerSize() {
    std::shared_lock<std::shared_mutex> lock(latch_);
    return replacer_->Size();
  }

  /**
   * Get the size of buffer pool (Added by Jigao)
   * @return the size of buffer pool
   */
  inline size_t GetPoolSize() {
    return pool_size_;
  }

  /**
   * Get the size of page table (Added by Jigao)
   * @return the size of page table
   */
  inline size_t GetPageTableSize() {
    std::shared_lock<std::shared_mutex> lock(latch_);
    return page_table_->GetSize();
  }

  /**
   * Check the existence of a page in buffer pool (Added by Jigao)
   * @return true for the page loaded in buffer pool, otherwise false
   */
  inline bool FindInBuffer(page_id_t page_id) {
    Page *page = nullptr;
    std::shared_lock<std::shared_mutex> lock(latch_);
    return page_table_->Find(page_id, page);
  }

private:
  size_t pool_size_; // number of pages in buffer pool
  Page *pages_;      // array of pages
  DiskManager *disk_manager_;
  LogManager *log_manager_;
  HashTable<page_id_t, Page *> *page_table_; // to keep track of pages
  Replacer<Page *> *replacer_;   // to find an unpinned page for replacement
  std::list<Page *> *free_list_; // to find a free page for replacement
  std::shared_mutex latch_;             // to protect shared data structure

  /**
   * find a replacement entry from either free list or replacer.
   * This function is not thread safe, SHOULD BE CALLED WITH PROTECTION OF MUTEX
   */
  Page *GetVictimPage();

  /**
   * check if all pages are pinned
   * This function is not thread safe, SHOULD BE CALLED WITH PROTECTION OF MUTEX
   */
  bool IsAllPinned() {
    for (size_t i = 0; i < pool_size_; i++) {
      if (pages_[i].pin_count_ != 0) return false;
    }
    return true;
  }

  };
} // namespace cmudb
