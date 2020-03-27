#include "buffer/buffer_pool_manager.h"

namespace cmudb {

/*
 * BufferPoolManager Constructor
 * When log_manager is nullptr, logging is disabled (for test purpose)
 * WARNING: Do Not Edit This Function
 */
BufferPoolManager::BufferPoolManager(size_t pool_size,
                                                 DiskManager *disk_manager,
                                                 LogManager *log_manager)
    : pool_size_(pool_size), disk_manager_(disk_manager),
      log_manager_(log_manager) {
  // a consecutive memory space for buffer pool
  pages_ = new Page[pool_size_];
  page_table_ = new ExtendibleHash<page_id_t, Page *>(BUCKET_SIZE);
  replacer_ = new LRUReplacer<Page *>;
  free_list_ = new std::list<Page *>;

  // put all the pages into free list
  for (size_t i = 0; i < pool_size_; ++i) {
    free_list_->push_back(&pages_[i]);
  }
}

/*
 * BufferPoolManager Deconstructor
 * WARNING: Do Not Edit This Function
 */
BufferPoolManager::~BufferPoolManager() {
  delete[] pages_;
  delete page_table_;
  delete replacer_;
  delete free_list_;
}

/**
 * 1. search hash table.
 *  1.1 if exist, pin the page and return immediately
 *  1.2 if no exist, find a replacement entry from either free list or lru
 *      replacer. (NOTE: always find from free list first)
 * 2. If the entry chosen for replacement is dirty, write it back to disk.
 * 3. Delete the entry for the old page from the hash table and insert an
 * entry for the new page.
 * 4. Update page metadata, read page content from disk file and return page
 * pointer
 */
Page *BufferPoolManager::FetchPage(page_id_t page_id) {
  assert(page_id != INVALID_PAGE_ID);
  std::lock_guard<std::mutex> lg(latch_);
  // 1. search hash table.
  Page *page = nullptr;
  page_table_->Find(page_id, page);
  // 1.1 if exist, pin the page and return immediately
  if (page != nullptr) {
    page->pin_count_++;
    replacer_->Erase(page);
    return page;
  }

  // 1.2 if no exist, find a replacement entry from either free list or lru replacer.
  //     (NOTE: always find from free list first)
  // 2. If the entry chosen for replacement is dirty, write it back to disk.
  // 3. Delete the entry for the old page from the hash table and insert an entry for the new page.
  page = GetVictimPage();

  // 4. Update page metadata, read page content from disk file and return page pointer
  page->page_id_ = page_id;
  page->pin_count_ = 1;
  disk_manager_->ReadPage(page_id, page->GetData());
  page_table_->Insert(page_id, page);
  replacer_->Erase(page);
  return page;
}

/*
 * Implementation of unpin page
 * if pin_count>0, decrement it and if it becomes zero, put it back to
 * replacer if pin_count<=0 before this call, return false. is_dirty: set the
 * dirty flag of this page
 */
bool BufferPoolManager::UnpinPage(page_id_t page_id, bool is_dirty) {
  assert(page_id != INVALID_PAGE_ID);
  std::lock_guard<std::mutex> lg(latch_);
  Page *page = nullptr;
  page_table_->Find(page_id, page);
  if (page == nullptr) return true;
  if (is_dirty) {
    page->is_dirty_ = true;
  }
  // if pin_count<=0 before this call, return false
  if (page->GetPinCount() <= 0) {
    return false;
  } else {
    // if pin_count>0, decrement it and if it becomes zero, put it back to replacer
    const auto pin_count = --page->pin_count_;
    if (pin_count == 0) {
      replacer_->Insert(page);
    }
    return true;
  }
}

/*
 * Used to flush a particular page of the buffer pool to disk. Should call the
 * write_page method of the disk manager
 * if page is not found in page table, return false
 * NOTE: make sure page_id != INVALID_PAGE_ID
 */
bool BufferPoolManager::FlushPage(page_id_t page_id) {
  assert(page_id != INVALID_PAGE_ID);
  std::lock_guard<std::mutex> lg(latch_);
  Page *page = nullptr;
  page_table_->Find(page_id, page);
  // if page is not found in page table, return false
  if (page == nullptr) return false;
  // call the write_page method of the disk manager
  disk_manager_->WritePage(page_id, page->GetData());
  return true;
}

/**
 * User should call this method for deleting a page. This routine will call
 * disk manager to deallocate the page. First, if page is found within page
 * table, buffer pool manager should be reponsible for removing this entry out
 * of page table, reseting page metadata and adding back to free list. Second,
 * call disk manager's DeallocatePage() method to delete from disk file. If
 * the page is found within page table, but pin_count != 0, return false
 */
 //TODO: 比较2018
bool BufferPoolManager::DeletePage(page_id_t page_id) {
  assert(page_id != INVALID_PAGE_ID);
   std::lock_guard<std::mutex> lg(latch_);
   // 1. check, if page is within page table
  Page *page = nullptr;
  page_table_->Find(page_id, page);
  if (page != nullptr) {
    // 1.1.0 If the page is found within page table, but pin_count != 0, return false
    if (page->pin_count_ != 0)
      return false;
    // 1.1.1 in page table, remove this entry out of page table, reset page metadata and add back to free list.
    page_table_->Remove(page_id);
    replacer_->Erase(page);
    page->ResetMemory();
    page->page_id_ = INVALID_PAGE_ID;
    free_list_->emplace_back(page);
    // 1.1.2 call disk manager's DeallocatePage() method to delete from disk file
    disk_manager_->DeallocatePage(page_id);
    return true;
  } else {
    // 1.2.1 call disk manager's DeallocatePage() method to delete from disk file
    disk_manager_->DeallocatePage(page_id);
    return true;
  }
}

/**
 * User should call this method if needs to create a new page. This routine
 * will call disk manager to allocate a page.
 * Buffer pool manager should be responsible to choose a victim page either
 * from free list or lru replacer(NOTE: always choose from free list first),
 * update new page's metadata, zero out memory and add corresponding entry
 * into page table. return nullptr if all the pages in pool are pinned
 */
Page *BufferPoolManager::NewPage(page_id_t &page_id) {
  std::lock_guard<std::mutex> lg(latch_);
  // 1. choose a victim page either from free list or lru replacer
  //    (NOTE: always choose from free list first)

  Page *page = GetVictimPage();
  if (page == nullptr) {
    // return nullptr if all the pages in pool are pinned
    page_id = INVALID_PAGE_ID;
    return nullptr;
  }

  // 2. call disk manager to allocate a page
  page_id = disk_manager_->AllocatePage();
  // 3. update new page's metadata, zero out memory and add corresponding entry into page table.
  page->page_id_ = page_id;
  page->pin_count_ = 1;
  page_table_->Insert(page_id, page);
  return page;
}

/**
 * find a replacement entry from either free list or lru replacer.
 * (NOTE: always find from free list first)
 * If the entry chosen for replacement is dirty, write it back to disk.
 * This function is not thread safe, SHOULD BE CALLED WITH PROTECTION OF MUTEX
 * @return the victim page as pointer, return nullptr if all the pages in pool are pinned
 */
Page *BufferPoolManager::GetVictimPage() {
  if (!free_list_->empty()) {
    Page *page = free_list_->front();
    assert(page->GetPageId() == INVALID_PAGE_ID);
    assert(page->GetPinCount() == 0);
    free_list_->pop_front();
    return page;
  } else if (replacer_->Size() != 0) {
    Page *page = nullptr;
    replacer_->Victim(page);
    if (page->is_dirty_) {
      disk_manager_->WritePage(page->page_id_, page->GetData());
    }
    page_table_->Remove(page->page_id_);
    page->ResetMemory();
    assert(page->GetPinCount() == 0);
    return page;
  } else {
    return nullptr;
  }
}


} // namespace cmudb
