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

// CLASS HEADER
#include <dali-ui-foundation/integration-api/page-scroll-view-impl.h>

// EXTERNAL INCLUDES
#include <algorithm>
#include <cmath>
// INTERNAL INCLUDES
#include <dali/devel-api/object/property-helper-devel.h>
#include <dali/devel-api/object/type-registry.h>
#include <dali/integration-api/debug.h>

namespace DALI_NAMESPACE
{

namespace Ui
{

namespace Integration
{

namespace
{
#if defined(DEBUG_ENABLED)
Debug::Filter* gLogFilter = Debug::Filter::New(Debug::NoLogging, false, "LOG_PAGE_SCROLL");
#endif

// Attachment storage preserves the published implementation object's size. A layout can
// report width, height and viewport separately; only a new geometry may rederive a page.
const AttachmentId PAGE_GEOMETRY = AttachmentId::Alloc();
struct PageGeometry
{
  float           primary{0};
  float           pageLength{0};
  ScrollDirection direction{ScrollDirection::Vertical};
  bool            valid{false};
};
PageGeometry& LastPageGeometry(Ui::View view)
{
  auto* geometry = view.GetAttachment<PageGeometry>(PAGE_GEOMETRY);
  if(!geometry)
  {
    view.SetAttachment(PAGE_GEOMETRY, Dali::MakeUnique<PageGeometry>());
    geometry = view.GetAttachment<PageGeometry>(PAGE_GEOMETRY);
  }

  DALI_ASSERT_ALWAYS(geometry && "PageScrollView geometry attachment creation failed");
  return *geometry;
}

// Request policy belongs only to the internal repositioning that created it. A
// public request from a callback never inherits NotifyPages/layout suppression.
const AttachmentId PAGE_REQUEST_STATE = AttachmentId::Alloc();
struct PageRequestState
{
  uint64_t targetOwner{0u};
  uint64_t deferredOwner{0u};
  uint64_t notificationEpoch{0u};
  int      queuedPage{-2};
  int      queuedCount{-2};
};

PageRequestState& GetPageRequestState(Ui::View view)
{
  auto* state = view.GetAttachment<PageRequestState>(PAGE_REQUEST_STATE);
  if(!state)
  {
    view.SetAttachment(PAGE_REQUEST_STATE, Dali::MakeUnique<PageRequestState>());
    state = view.GetAttachment<PageRequestState>(PAGE_REQUEST_STATE);
  }
  DALI_ASSERT_ALWAYS(state && "PageScrollView request attachment creation failed");
  return *state;
}

// Only geometry and index adjustment run under this scope, never public callbacks.
struct NotifyInProgressScope
{
  explicit NotifyInProgressScope(bool& flag)
  : mFlag(flag),
    mPrevious(flag)
  {
    mFlag = true;
  }
  ~NotifyInProgressScope()
  {
    mFlag = mPrevious;
  }
  NotifyInProgressScope(const NotifyInProgressScope&)            = delete;
  NotifyInProgressScope& operator=(const NotifyInProgressScope&) = delete;

  bool& mFlag;
  bool  mPrevious;
};

constexpr float PAGE_SNAP_DURATION  = 0.30f; ///< Fixed snap animation duration (seconds)
constexpr float PAGE_SNAP_THRESHOLD = 0.30f; ///< Fraction of page crossed to trigger page advance on slow drag

BaseHandle Create()
{
  return BaseHandle();
}

DALI_TYPE_REGISTRATION_BEGIN_FULL(Ui::PageScrollView, Ui::Integration::PageScrollViewImpl, Ui::ScrollView, Create)
DALI_TYPE_REGISTRATION_END()

} // namespace

PageScrollViewImplPtr PageScrollViewImpl::New()
{
  return PageScrollViewImplPtr(new PageScrollViewImpl());
}

PageScrollViewImpl::PageScrollViewImpl()
: ScrollViewImpl()
{
}

PageScrollViewImpl::~PageScrollViewImpl()
{
  // Notify observers (e.g. PageIndicator) before any members are destroyed.
  // Handlers must not call back into this object.
  if(!mDestroyingSignal.Empty())
    mDestroyingSignal.Emit();
}

void PageScrollViewImpl::OnInitialize()
{
  ScrollViewImpl::OnInitialize();

  SetScrollCompletionHandler([this](uint64_t request, ScrollCompletion reason)
  {
    OnScrollCompleted(request, reason);
  });
}

void PageScrollViewImpl::SetPageSize(const Vector2& size)
{
  auto lifetime = Self();
  if(mPageSize == size) return;
  mPageSize = size;

  // Page size change immediately affects GetPageCount() and page boundaries.
  // If layout has already run (primary scrollable dimension > 0), re-derive
  // mCurrentPage from the current scroll position with the new page size and
  // notify observers if count or page changed.
  // Pre-layout: the primary dimension is 0 here; OnScrollableAreaChanged from
  // the first layout pass will emit the correct values.
  ScrollDirection dir     = GetScrollDirection();
  float           primary = (dir == ScrollDirection::Horizontal) ? GetScrollableWidth() : GetScrollableHeight();
  if(primary < 1.0f) return;
  const Vector2 effective  = GetEffectivePageSize();
  const float   pageLength = dir == ScrollDirection::Horizontal ? effective.x : effective.y;
  if(pageLength >= 1.0f)
    LastPageGeometry(Ui::View::DownCast(Self())) = {primary, pageLength, dir, true};

  int newCount = GetPageCount();
  int newPage  = (newCount > 0)
                   ? std::max(0, std::min(newCount - 1, PageForScrollPosition(GetScrollPosition())))
                   : 0;

  if(newCount != mLastNotifiedPageCount || newPage != mCurrentPage)
  {
    mCurrentPage = newPage;
    EmitPageChanged(GetCurrentPage(), newCount);
  }
}

Vector2 PageScrollViewImpl::GetPageSize() const
{
  return mPageSize;
}

int PageScrollViewImpl::GetCurrentPage() const
{
  // Returns -1 when the view is empty (GetPageCount() == 0) to distinguish
  // "no pages" from "first page (index 0)".
  return (GetPageCount() <= 0) ? -1 : mCurrentPage;
}

int PageScrollViewImpl::GetPageCount() const
{
  // mExpectedPageCount >= 0: set by NotifyPages* — use as authoritative count
  // (including 0, which means "truly empty after explicit removal").
  // mExpectedPageCount == -1: sentinel, fall through to layout-based computation.
  if(mExpectedPageCount >= 0)
    return mExpectedPageCount;

  Vector2         pageSize = GetEffectivePageSize();
  ScrollDirection dir      = GetScrollDirection();

  float contentLen = (dir == ScrollDirection::Horizontal)
                       ? GetScrollableWidth()
                       : GetScrollableHeight();
  float pageLen    = (dir == ScrollDirection::Horizontal) ? pageSize.x : pageSize.y;

  // Viewport not yet measured — return 1 as a conservative pre-layout value.
  if(pageLen < 1.0f) return 1;

  int count = static_cast<int>(std::ceil(contentLen / pageLen));
  return std::max(0, count);
}

void PageScrollViewImpl::ScrollToPage(int page, bool animate)
{
  ScrollToPageRequest(page, animate, false);
}

uint64_t PageScrollViewImpl::ScrollToPageRequest(int page, bool animate, bool deferNotification)
{
  auto           lifetime = Self();
  const uint64_t request  = BeginScrollRequest();
  CancelScrollAnimation(ScrollCompletion::Replaced);
  if(!IsCurrentScrollRequest(request)) return request;

  const int     total      = GetPageCount();
  const int     target     = std::max(0, std::min(total - 1, page));
  const Vector2 pageSize   = GetEffectivePageSize();
  const float   pageLength = GetScrollDirection() == ScrollDirection::Horizontal ? pageSize.x : pageSize.y;
  auto&         state      = GetPageRequestState(Ui::View::DownCast(lifetime));
  if(mExpectedPageCount >= 0 && pageLength < 1.0f)
  {
    mSnapTargetPage   = -1;
    state.targetOwner = state.deferredOwner = 0u;
    mCurrentPage                            = target;
    if(!deferNotification) EmitPageChanged(GetCurrentPage(), total);
    return request;
  }

  state.targetOwner            = request;
  state.deferredOwner          = deferNotification ? request : 0u;
  mSnapTargetPage              = target;
  const Vector2 targetPosition = ScrollPositionForPage(target);
  if(animate)
    ScrollToWithDurationForRequest(targetPosition, PAGE_SNAP_DURATION, request);
  else
    ScrollToForRequest(targetPosition, false, request);
  return request;
}

PageScrollableInterface::PageChangedSignalType& PageScrollViewImpl::PageChangedSignal()
{
  return mPageChangedSignal;
}

PageScrollableInterface::DestroyingSignalType& PageScrollViewImpl::DestroyingSignal()
{
  return mDestroyingSignal;
}

// ─── ScrollViewImpl hook ───────────────────────────────────────────────────

void PageScrollViewImpl::OnBeforeScrollAnimation(Vector2& targetPosition, float& durationSec)
{
  int total = GetPageCount();
  if(total <= 1) return;

  int snapPage;

  if(durationSec < 0.0001f)
  {
    // Slow drag (speed < FLING_VMIN): decide snap direction from how far the
    // current position has moved past the nearest page boundary.
    Vector2         curPos = GetScrollPosition();
    Vector2         pgSize = GetEffectivePageSize();
    ScrollDirection dir    = GetScrollDirection();

    float pos     = (dir == ScrollDirection::Horizontal) ? curPos.x : curPos.y;
    float pageLen = (dir == ScrollDirection::Horizontal) ? pgSize.x : pgSize.y;

    if(pageLen < 1.0f)
    {
      return;
    }

    int   basePage = (mSnapTargetPage >= 0) ? mSnapTargetPage : mCurrentPage;
    float basePos  = (dir == ScrollDirection::Horizontal)
                       ? ScrollPositionForPage(basePage).x
                       : ScrollPositionForPage(basePage).y;
    float offset   = (pos - basePos) / pageLen; // signed distance from basePage's actual position

    // Advance one page if the user has dragged past PAGE_SNAP_THRESHOLD of a
    // page width from the base page.  Measuring from basePage (not round()) avoids
    // snapping backward in the 50–70 % region where round() already rounds up.
    if(offset > PAGE_SNAP_THRESHOLD)
      snapPage = basePage + 1;
    else if(offset < -PAGE_SNAP_THRESHOLD)
      snapPage = basePage - 1;
    else
      snapPage = basePage;

    snapPage = std::max(0, std::min(total - 1, snapPage));
  }
  else
  {
    Vector2         pgSize = GetEffectivePageSize();
    ScrollDirection dir    = GetScrollDirection();

    float target  = (dir == ScrollDirection::Horizontal) ? targetPosition.x : targetPosition.y;
    float pageLen = (dir == ScrollDirection::Horizontal) ? pgSize.x : pgSize.y;

    if(pageLen < 1.0f) return;

    int projected = static_cast<int>(std::round(target / pageLen));
    int basePage  = (mSnapTargetPage >= 0) ? mSnapTargetPage : mCurrentPage;
    snapPage      = std::max(basePage - 1, std::min(basePage + 1, projected));
    snapPage      = std::max(0, std::min(total - 1, snapPage));
  }

  GetPageRequestState(Ui::View::DownCast(Self())).targetOwner = GetCurrentScrollRequest();
  mSnapTargetPage                                             = snapPage;
  targetPosition                                              = ScrollPositionForPage(snapPage);
  durationSec                                                 = PAGE_SNAP_DURATION;

  DALI_LOG_INFO(gLogFilter, Debug::Verbose, "[PageScrollView] OnBeforeScrollAnimation → page %d pos=(%.1f, %.1f)\n",
                snapPage, targetPosition.x, targetPosition.y);
}

// ─── Private helpers ──────────────────────────────────────────────────────

Vector2 PageScrollViewImpl::GetEffectivePageSize() const
{
  if(mPageSize.x > 0.0f || mPageSize.y > 0.0f)
    return mPageSize;

  return Vector2(GetViewportWidth(), GetViewportHeight());
}

Vector2 PageScrollViewImpl::ScrollPositionForPage(int page) const
{
  Vector2         pgSize = GetEffectivePageSize();
  ScrollDirection dir    = GetScrollDirection();

  if(dir == ScrollDirection::Horizontal)
    return AdjustScrollPosition(Vector2(page * pgSize.x, 0.0f));
  else
    return AdjustScrollPosition(Vector2(0.0f, page * pgSize.y));
}

int PageScrollViewImpl::PageForScrollPosition(const Vector2& scrollPos) const
{
  Vector2         pgSize = GetEffectivePageSize();
  ScrollDirection dir    = GetScrollDirection();

  float pos     = (dir == ScrollDirection::Horizontal) ? scrollPos.x : scrollPos.y;
  float pageLen = (dir == ScrollDirection::Horizontal) ? pgSize.x : pgSize.y;

  if(pageLen < 1.0f) return 0;

  return std::max(0, std::min(GetPageCount() - 1, static_cast<int>(std::round(pos / pageLen))));
}

void PageScrollViewImpl::CommitPage(int newPage)
{
  mCurrentPage = newPage;
  EmitPageChanged(GetCurrentPage(), GetPageCount());
}

void PageScrollViewImpl::EmitPageChanged(int currentPage, int pageCount)
{
  auto           lifetime = Self();
  auto&          state    = GetPageRequestState(Ui::View::DownCast(lifetime));
  const uint64_t epoch    = GetScrollNotificationEpoch();
  if(state.notificationEpoch == epoch && state.queuedPage == currentPage && state.queuedCount == pageCount) return;
  state.notificationEpoch = epoch;
  state.queuedPage        = currentPage;
  state.queuedCount       = pageCount;
  mLastNotifiedPageCount  = pageCount;
  QueueScrollNotification([this, currentPage, pageCount]
  {
    mPageChangedSignal.Emit(currentPage, pageCount);
  });
}

void PageScrollViewImpl::OnScrollableAreaChanged()
{
  auto lifetime = Self();
  if(mNotifyInProgress) return;
  const ScrollDirection dir      = GetScrollDirection();
  const Vector2         pageSize = GetEffectivePageSize();
  const float           primary  = dir == ScrollDirection::Horizontal ? GetScrollableWidth() : GetScrollableHeight();
  const float           pageLen  = dir == ScrollDirection::Horizontal ? pageSize.x : pageSize.y;
  if(pageLen < 1.0f) return;
  auto& geometry = LastPageGeometry(Ui::View::DownCast(lifetime));
  if(primary < 1.0f && (!geometry.valid || geometry.direction != dir)) return;
  const bool geometryChanged = !geometry.valid || geometry.direction != dir ||
                               std::abs(geometry.primary - primary) >= 0.01f ||
                               std::abs(geometry.pageLength - pageLen) >= 0.01f;
  geometry = {primary, pageLen, dir, true};

  const bool pendingSelection = mExpectedPageCount >= 0;
  mExpectedPageCount          = -1;
  bool selectionKept          = false;
  bool applyReplaced          = false;
  if(pendingSelection && !IsScrolling() && mSnapTargetPage < 0)
  {
    const int total = GetPageCount();
    if(total > 0)
    {
      mCurrentPage          = std::max(0, std::min(total - 1, mCurrentPage));
      selectionKept         = true;
      const Vector2 target  = ScrollPositionForPage(mCurrentPage);
      const Vector2 current = GetLiveScrollPosition();
      if(std::abs(target.x - current.x) >= 0.5f || std::abs(target.y - current.y) >= 0.5f)
      {
        const uint64_t request = ScrollToPageRequest(mCurrentPage, false, true);
        // A callback may have replaced the apply. A completed newer request has
        // already committed its logical page; an active one owns the target/live offset.
        applyReplaced = !IsCurrentScrollRequest(request);
        if(applyReplaced) selectionKept = !IsScrolling();
      }
    }
  }

  const int  newCount     = GetPageCount();
  const bool countChanged = newCount != mLastNotifiedPageCount;
  const bool settled      = !IsScrolling() && mSnapTargetPage < 0;
  int        newPage      = mCurrentPage;
  if(!selectionKept && (applyReplaced || countChanged || (geometryChanged && settled)))
    newPage = mSnapTargetPage >= 0 ? mSnapTargetPage : PageForScrollPosition(GetLiveScrollPosition());
  newPage = newCount > 0 ? std::max(0, std::min(newCount - 1, newPage)) : 0;
  CommitPage(newPage);
}

void PageScrollViewImpl::OnScrollCompleted(uint64_t request, ScrollCompletion reason)
{
  auto&      state      = GetPageRequestState(Ui::View::DownCast(Self()));
  const bool ownsTarget = state.targetOwner == request;
  const bool deferred   = state.deferredOwner == request;
  const int  target     = ownsTarget ? mSnapTargetPage : -1;
  if(ownsTarget)
  {
    mSnapTargetPage   = -1;
    state.targetOwner = 0u;
  }
  if(deferred) state.deferredOwner = 0u;
  if(reason == ScrollCompletion::Replaced) return;
  // A free-scroll no-op has no arrival transition. In particular it must not
  // demote an explicitly selected partial page by rounding the unchanged offset.
  if(reason == ScrollCompletion::NoOp && !ownsTarget) return;

  const Vector2 pageSize   = GetEffectivePageSize();
  const float   pageLength = GetScrollDirection() == ScrollDirection::Horizontal ? pageSize.x : pageSize.y;
  // A physical position cannot resolve a pending logical selection before measurement.
  if(pageLength < 1.0f && mExpectedPageCount >= 0) return;
  if(!deferred && reason != ScrollCompletion::NoOp) mExpectedPageCount = -1;
  int       page  = target >= 0 && reason != ScrollCompletion::Interrupted ? target : PageForScrollPosition(GetLiveScrollPosition());
  const int total = GetPageCount();
  mCurrentPage    = total > 0 ? std::max(0, std::min(total - 1, page)) : 0;
  if(!deferred) EmitPageChanged(GetCurrentPage(), total);
}

void PageScrollViewImpl::OnScrollFinished(Ui::ScrollView /*scrollView*/)
{
  // Kept as a nonvirtual ABI wrapper; internal completion never subscribes to a signal.
  OnScrollCompleted(GetCurrentScrollRequest(), ScrollCompletion::Completed);
}

void PageScrollViewImpl::PublishPageAfterNotify(uint64_t request)
{
  if(!IsCurrentScrollRequest(request) && IsScrolling())
    mCurrentPage = mSnapTargetPage >= 0 ? mSnapTargetPage : PageForScrollPosition(GetLiveScrollPosition());
  const int total = GetPageCount();
  mCurrentPage    = total > 0 ? std::max(0, std::min(total - 1, mCurrentPage)) : 0;
  EmitPageChanged(GetCurrentPage(), total);
}

// ─── Dynamic page insertion / removal ────────────────────────────────────────

void PageScrollViewImpl::NotifyPagesInserted(int atIndex, int insertedCount)
{
  auto lifetime = Self();
  if(insertedCount <= 0) return;

  int oldTotal = GetPageCount();

  // P3: clamp atIndex to valid insertion range [0, oldTotal].
  atIndex = std::max(0, std::min(oldTotal, atIndex));

  // GetPageCount() may still reflect the pre-insert layout value — use it
  // as the "before" count and add insertedCount to get the expected total.
  int newTotal       = oldTotal + insertedCount;
  mExpectedPageCount = newTotal;

  // Suppress only geometry callbacks while adjusting the count and logical index.
  {
    NotifyInProgressScope notify(mNotifyInProgress);

    // Proactively update the scroll-view's bounds so that AdjustScrollPosition
    // in the subsequent ScrollToPage call uses the correct (post-insert) range,
    // not the stale layout-measured width/height.
    // P1: skip when pageSize is zero (pre-layout); Arrange() will set correct
    //     bounds on the next layout pass, and the page selected below is applied
    //     to the scroll position by OnScrollableAreaChanged once they exist.
    Vector2         pgSize = GetEffectivePageSize();
    ScrollDirection dir    = GetScrollDirection();
    float           pgLen  = (dir == ScrollDirection::Horizontal) ? pgSize.x : pgSize.y;
    if(pgLen > 0.0f)
    {
      if(dir == ScrollDirection::Horizontal)
        SetScrollableWidth(pgLen * newTotal);
      else
        SetScrollableHeight(pgLen * newTotal);
    }

    // If the insertion happened at or before the current page, the content
    // page the user was viewing has shifted right — track it.
    if(atIndex <= mCurrentPage)
      mCurrentPage += insertedCount;
    mCurrentPage = std::max(0, std::min(newTotal - 1, mCurrentPage));
  }
  const uint64_t request = ScrollToPageRequest(mCurrentPage, false, true);

  DALI_LOG_INFO(gLogFilter, Debug::Verbose,
                "[PageScrollView] NotifyPagesInserted atIndex=%d count=%d → page %d/%d\n",
                atIndex, insertedCount, mCurrentPage, newTotal);

  // P2: emit GetCurrentPage() so that -1 is propagated when the view is empty.
  PublishPageAfterNotify(request);
  // mExpectedPageCount is cleared by OnScrollableAreaChanged when the natural
  // layout pass fires (mNotifyInProgress is false at that point), which also
  // applies the page selected here if this ran before the first layout.
}

void PageScrollViewImpl::NotifyPagesRemoved(int atIndex, int removedCount)
{
  auto lifetime = Self();
  if(removedCount <= 0) return;

  int oldTotal = GetPageCount(); // may be pre-layout but is our best "before" value

  // P3: clamp to valid removal range so that out-of-bounds calls don't corrupt state.
  atIndex      = std::max(0, std::min(oldTotal > 0 ? oldTotal - 1 : 0, atIndex));
  removedCount = std::min(removedCount, oldTotal - atIndex);
  if(removedCount <= 0) return;

  int newTotal       = std::max(0, oldTotal - removedCount); // 0 = valid empty state
  mExpectedPageCount = newTotal;

  {
    NotifyInProgressScope notify(mNotifyInProgress);

    // P1: skip when pageSize is zero (pre-layout); Arrange() will set correct
    //     bounds on the next layout pass, and the page selected below is applied
    //     to the scroll position by OnScrollableAreaChanged once they exist.
    //     Post-layout this always executes.
    Vector2         pgSize = GetEffectivePageSize();
    ScrollDirection dir    = GetScrollDirection();
    float           pgLen  = (dir == ScrollDirection::Horizontal) ? pgSize.x : pgSize.y;
    if(pgLen > 0.0f)
    {
      if(dir == ScrollDirection::Horizontal)
        SetScrollableWidth(pgLen * newTotal);
      else
        SetScrollableHeight(pgLen * newTotal);
    }

    int atEnd = atIndex + removedCount; // first index past the removed range

    if(atIndex > mCurrentPage)
    {
      // Removal entirely after current page — no index change.
    }
    else if(atEnd <= mCurrentPage)
    {
      // Removal entirely before current page — shift index left.
      mCurrentPage -= removedCount;
    }
    else
    {
      // Removal overlaps the current page — snap to the start of the removed
      // range (or the last valid page if that's past the new end).
      mCurrentPage = atIndex;
    }

    mCurrentPage = (newTotal > 0) ? std::max(0, std::min(newTotal - 1, mCurrentPage)) : 0;
  }
  // Reposition after releasing the geometry-only guard, including the empty case.
  const uint64_t request = ScrollToPageRequest(mCurrentPage, false, true);

  DALI_LOG_INFO(gLogFilter, Debug::Verbose,
                "[PageScrollView] NotifyPagesRemoved atIndex=%d count=%d → page %d/%d\n",
                atIndex, removedCount, mCurrentPage, newTotal);

  // P2: emit GetCurrentPage() so that -1 is propagated when the view is empty.
  PublishPageAfterNotify(request);
  // mExpectedPageCount is cleared by OnScrollableAreaChanged when the natural
  // layout pass fires (mNotifyInProgress is false at that point), which also
  // applies the page selected here if this ran before the first layout.
}

} // namespace Integration

} // namespace Ui

} //namespace DALI_NAMESPACE
