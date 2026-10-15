#pragma once

/*
 * Copyright (c) 2026 Samsung Electronics Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

// INTERNAL INCLUDES
#include <dali-ui-foundation/integration-api/scroll-view-impl.h>
#include <dali-ui-foundation/public-api/views/scroll/page-scroll-view.h>
#include <dali-ui-foundation/public-api/views/scroll/page-scrollable-interface.h>

namespace DALI_NAMESPACE
{

namespace Ui
{

namespace Integration
{

class PageScrollViewImpl;
using PageScrollViewImplPtr = IntrusivePtr<PageScrollViewImpl>;

/**
 * @brief Internal implementation of PageScrollView.
 *
 * Subclasses ScrollViewImpl to add page-snap behaviour:
 *  - OnBeforeScrollAnimation() is overridden to redirect any fling or slow-drag
 *    finish to the nearest page boundary.
 *  - A PageChangedSignal is emitted whenever the settled page index changes.
 *
 * External libraries that need only public-api access should implement
 * PageScrollableInterface on their own View subclass instead of inheriting this class.
 *
 * @note ABI note: do NOT reorder or remove virtual methods after first publication.
 */
class DALI_UI_API PageScrollViewImpl : public ScrollViewImpl, public PageScrollableInterface
{
public:
  /**
   * @brief Creates a new PageScrollViewImpl.
   */
  static PageScrollViewImplPtr New();

protected:
  /**
   * @brief Destructor.
   */
  ~PageScrollViewImpl() override;

  /**
   * @brief Constructor.
   */
  PageScrollViewImpl();

public: // From ViewImpl
  /**
   * @brief Connects post-scroll signals after the base class is initialized.
   */
  void OnInitialize() override;

public: // Page API
  /**
   * @brief Sets the explicit page size.
   *
   * Only the component on the active scroll axis is used.
   * Set to (0, 0) to derive the page size from the current viewport.
   */
  void SetPageSize(const Vector2& size);

  /**
   * @brief Gets the explicitly configured page size ((0,0) means auto).
   */
  Vector2 GetPageSize() const;

  // PageScrollableInterface

  /**
   * @brief Returns the zero-based current page index, or -1 if empty.
   */
  int GetCurrentPage() const override;

  /**
   * @brief Returns the total page count derived from content / effective page size.
   *
   * When NotifyPagesInserted / NotifyPagesRemoved has been called but the
   * layout pass has not yet run, returns the pre-computed expected count
   * stored by those methods so that PageChangedSignal consumers see a
   * consistent value immediately.
   */
  int GetPageCount() const override;

  /**
   * @brief Scrolls to the given page with an optional animation.
   */
  void ScrollToPage(int page, bool animate = true) override;

  /**
   * @brief Call after dynamically inserting pages into the scroll content.
   *
   * Adjusts the current-page index (shift right if the insertion was at or
   * before the current page), scrolls instantly to keep the same content
   * page visible, and emits PageChangedSignal with the updated state.
   *
   * Must be called AFTER the pages have been added to the content layout.
   * The scroll-view layout pass may not have run yet — this method
   * computes the new page count from mScrollableSize + inserted count to
   * avoid the timing ambiguity.
   *
   * @param[in] atIndex       Zero-based index at which pages were inserted.
   * @param[in] insertedCount Number of pages inserted (default 1).
   */
  void NotifyPagesInserted(int atIndex, int insertedCount = 1);

  /**
   * @brief Call after dynamically removing pages from the scroll content.
   *
   * Adjusts the current-page index, snaps to the nearest valid page if
   * the current page was removed, and emits PageChangedSignal.
   *
   * Must be called AFTER the pages have been removed from the content layout.
   *
   * @param[in] atIndex      Zero-based index of the first removed page.
   * @param[in] removedCount Number of pages removed (default 1).
   */
  void NotifyPagesRemoved(int atIndex, int removedCount = 1);

  /**
   * @brief Signal emitted when the settled page changes.
   */
  PageScrollableInterface::PageChangedSignalType& PageChangedSignal() override;

  /**
   * @brief Signal emitted from the destructor before this object is freed.
   *
   * Observers (e.g. PageIndicator) connect here to null any raw pointer they
   * hold to this object.  Do not call back into the sender from the handler.
   */
  PageScrollableInterface::DestroyingSignalType& DestroyingSignal() override;

protected:
  /**
   * @brief Overrides ScrollViewImpl hook to snap fling/drag targets to page boundaries.
   *
   * @note ABI note: do NOT reorder or remove after first publication.
   */
  void OnBeforeScrollAnimation(Vector2& targetPosition, float& durationSec) override;

  /**
   * @brief Reconciles measured geometry with the latest scroll request.
   *
   * A pending NotifyPages selection is applied once usable geometry exists. The
   * internal apply defers page publication only for its own request; a callback's
   * newer request owns the resulting selection. Count changes during an active
   * snap use its target, while active free scrolling uses the live position.
   * Repeated geometry callbacks leave settled partial-page selections unchanged.
   * Measured empty/nonempty transitions report (-1, 0) or the new page/count.
   *
   * @note ABI note: do NOT reorder or remove after first publication.
   */
  void OnScrollableAreaChanged() override;

private:
  /**
   * @brief Returns the effective page size.
   *
   * Uses mPageSize if non-zero; falls back to the current viewport dimensions.
   */
  Vector2 GetEffectivePageSize() const;

  /**
   * @brief Returns the scroll position that centers the given page in the viewport.
   */
  Vector2 ScrollPositionForPage(int page) const;

  /**
   * @brief Returns the page index that best matches the given scroll position.
   */
  int PageForScrollPosition(const Vector2& scrollPos) const;

  /**
   * @brief Updates mCurrentPage if changed and emits PageChangedSignal.
   */
  void CommitPage(int newPage);

  /**
   * @brief Queues a page/count snapshot, suppressing duplicate queued pairs.
   *
   * An aborted dispatch invalidates deduplication so a later request can resynchronize
   * observers. mLastNotifiedPageCount records the latest queued count.
   */
  void EmitPageChanged(int currentPage, int pageCount);

  uint64_t ScrollToPageRequest(int page, bool animate, bool deferNotification);
  void     OnScrollCompleted(uint64_t request, ScrollCompletion reason);
  void     PublishPageAfterNotify(uint64_t request);

  /** @brief Retained nonvirtual wrapper; not connected to a public signal. */
  void OnScrollFinished(Ui::ScrollView scrollView);

private:
  // Not copyable or movable
  PageScrollViewImpl(const PageScrollViewImpl&)            = delete;
  PageScrollViewImpl(PageScrollViewImpl&&)                 = delete;
  PageScrollViewImpl& operator=(const PageScrollViewImpl&) = delete;
  PageScrollViewImpl& operator=(PageScrollViewImpl&&)      = delete;

private:
  Vector2                                        mPageSize{0.0f, 0.0f};      ///< Explicit page size; (0,0) = use viewport
  int                                            mCurrentPage{0};            ///< Settled page index
  int                                            mSnapTargetPage{-1};        ///< Logical target awaiting completion; -1 = no explicit target
  int                                            mExpectedPageCount{-1};     ///< >=0: overrides layout-based count; -1: use layout (sentinel)
  int                                            mLastNotifiedPageCount{-2}; ///< Last queued page count; -2 = never queued
  bool                                           mNotifyInProgress{false};   ///< Suppress geometry callbacks during NotifyPages* index adjustment
  PageScrollableInterface::PageChangedSignalType mPageChangedSignal;         ///< Emitted on page settle
  PageScrollableInterface::DestroyingSignalType  mDestroyingSignal;          ///< Emitted from destructor for observer lifetime management
};

} // namespace Integration

} // namespace Ui

} //namespace DALI_NAMESPACE
