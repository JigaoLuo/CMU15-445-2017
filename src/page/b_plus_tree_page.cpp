/**
 * b_plus_tree_page.cpp
 */
#include "page/b_plus_tree_page.h"

namespace cmudb {

/*
 * Helper methods to get/set page type
 * Page type enum class is defined in b_plus_tree_page.h
 */
bool BPlusTreePage::IsLeafPage() const { return false; }
bool BPlusTreePage::IsRootPage() const { return false; }
void BPlusTreePage::SetPageType(IndexPageType page_type) {}

/*
 * Helper methods to get/set size (number of key/value pairs stored in that
 * page)
 */
int BPlusTreePage::GetSize() const { return 0; }
void BPlusTreePage::SetSize(int size) {}
void BPlusTreePage::IncreaseSize(int amount) {}

/*
 * Helper methods to get/set max size (capacity) of the page
 */
int BPlusTreePage::GetMaxSize() const { return 0; }
void BPlusTreePage::SetMaxSize(int size) {}

/*
 * Helper method to get min page size
 * Generally, min page size == max page size / 2
 */
int BPlusTreePage::GetMinSize() const { return 0; }

/*
 * Helper methods to get/set parent page id
 */
page_id_t BPlusTreePage::GetParentPageId() const { return 0; }
void BPlusTreePage::SetParentPageId(page_id_t parent_page_id) {}

/*
 * Helper methods to get/set self page id
 */
page_id_t BPlusTreePage::GetPageId() const { return 0; }
void BPlusTreePage::SetPageId(page_id_t page_id) {}

/*
 * Helper methods to set lsn
 */
void BPlusTreePage::SetLSN(lsn_t lsn) { lsn_ = lsn; }

} // namespace cmudb

// 实现
// https://github.com/jackwener/cmu-15445/blob/master/src/page/b_plus_tree_page.cpp
