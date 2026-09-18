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

#include <dali-ui-foundation/dali-ui-foundation.h>
#include <dali-ui-test-suite-utils.h>
#include <dali.h>
#include <stdlib.h>
#include <utility>
#include <vector>

using namespace Dali;
using namespace Dali::Ui;

void utc_dali_page_scroll_view_startup(void)
{
  test_return_value = TET_UNDEF;
}

void utc_dali_page_scroll_view_cleanup(void)
{
  test_return_value = TET_PASS;
}

// PageScrollView had no automated coverage. These tests pin the page-notification
// contract of OnScrollableAreaChanged and OnScrollFinished: what the view reports
// while a snap is in flight, when a viewport resize changes the page count, and
// when a page is selected before the first layout pass has any real dimensions.

namespace
{
const float PAGER_WIDTH   = 100.0f;
const float PAGER_HEIGHT  = 80.0f;
const float CONTENT_WIDTH = 500.0f;

/// Records every (page, count) pair PageChangedSignal reports, in order.
struct PageSignalRecorder : public ConnectionTracker
{
  void OnPageChanged(int page, int count)
  {
    signals.push_back(std::make_pair(page, count));
  }

  std::vector<std::pair<int, int>> signals;
};

/// A horizontal pager 100 x 80 over 500 units of content: five pages of 100.
/// The content is NOT yet on the scene, so no layout pass has run.
PageScrollView BuildPager(View& content)
{
  PageScrollView pager = PageScrollView::New();
  pager.SetScrollDirection(ScrollDirection::Horizontal);
  pager.SetRequestedWidth(PAGER_WIDTH);
  pager.SetRequestedHeight(PAGER_HEIGHT);

  content = View::New();
  content.SetRequestedWidth(CONTENT_WIDTH);
  content.SetRequestedHeight(PAGER_HEIGHT);
  pager.SetContent(content);

  return pager;
}

} // namespace

// A shrinking page count while a snap animation is in flight must report the page the
// scroll is heading to, not the stale settled page: the scroll position during an
// animated scroll is the (clamped) animation target. The subsequent OnScrollFinished
// then finds the same page and stays silent, so the whole resize reports exactly once.
int UtcDaliPageScrollViewCountShrinkDuringSnapP(void)
{
  UiTestApplication application;
  Window            window = application.GetWindow();

  View           content;
  PageScrollView pager = BuildPager(content);
  window.Add(pager);

  application.SendNotification();
  application.Render();

  DALI_TEST_EQUALS(pager.GetPageCount(), 5, TEST_LOCATION);

  PageSignalRecorder recorder;
  pager.PageChangedSignal().Connect(&recorder, &PageSignalRecorder::OnPageChanged);

  // Snap to the last page (scroll position 400, the clamped maximum).
  pager.ScrollToPage(4, true);
  application.SendNotification();
  application.Render(16);
  DALI_TEST_CHECK(pager.IsScrolling());

  // The viewport grows to 250, so the page length becomes 250 and the count 2. The
  // clamped animation target is 250, i.e. page 1.
  pager.SetRequestedWidth(250.0f);
  application.SendNotification();

  DALI_TEST_EQUALS(static_cast<int>(recorder.signals.size()), 1, TEST_LOCATION);
  DALI_TEST_EQUALS(recorder.signals[0].first, 1, TEST_LOCATION);
  DALI_TEST_EQUALS(recorder.signals[0].second, 2, TEST_LOCATION);

  // Let the snap animation finish. Its endpoint is outside the shrunk range, so the
  // settled position is repaired, and the snap target clamps onto the page already
  // reported - no second notification.
  application.Render(100);
  application.Render(100);
  application.Render(100);
  application.Render(100);
  application.SendNotification();

  DALI_TEST_CHECK(!pager.IsScrolling());
  DALI_TEST_EQUALS(pager.GetPageCount(), 2, TEST_LOCATION);
  DALI_TEST_EQUALS(pager.GetCurrentPage(), 1, TEST_LOCATION);
  DALI_TEST_EQUALS(pager.GetScrollPosition().x, 250.0f, 0.001f, TEST_LOCATION);
  DALI_TEST_EQUALS(static_cast<int>(recorder.signals.size()), 1, TEST_LOCATION);
  END_TEST;
}

// A viewport-only resize reaches the page view at all: a settled pager whose own width
// changes re-derives its count and page and reports them once.
int UtcDaliPageScrollViewViewportResizeNotifiesP(void)
{
  UiTestApplication application;
  Window            window = application.GetWindow();

  View           content;
  PageScrollView pager = BuildPager(content);
  window.Add(pager);

  application.SendNotification();
  application.Render();

  DALI_TEST_EQUALS(pager.GetPageCount(), 5, TEST_LOCATION);

  PageSignalRecorder recorder;
  pager.PageChangedSignal().Connect(&recorder, &PageSignalRecorder::OnPageChanged);

  pager.SetRequestedWidth(250.0f);
  application.SendNotification();

  DALI_TEST_EQUALS(pager.GetPageCount(), 2, TEST_LOCATION);
  DALI_TEST_EQUALS(static_cast<int>(recorder.signals.size()), 1, TEST_LOCATION);
  DALI_TEST_EQUALS(recorder.signals[0].first, 0, TEST_LOCATION);
  DALI_TEST_EQUALS(recorder.signals[0].second, 2, TEST_LOCATION);
  END_TEST;
}

// A page selected before the first layout pass can only be recorded: the page length is
// still 0, so the physical scroll position cannot be resolved. The first pass that has
// real dimensions must apply it AND keep it, rather than re-deriving page 0 from the
// scroll position that was never moved.
int UtcDaliPageScrollViewPreLayoutSelectionAppliedP(void)
{
  UiTestApplication application;
  Window            window = application.GetWindow();

  View           content;
  PageScrollView pager = BuildPager(content);

  PageSignalRecorder recorder;
  pager.PageChangedSignal().Connect(&recorder, &PageSignalRecorder::OnPageChanged);

  // Pre-layout the count reads as 1, so three insertions make four pages and shift the
  // current page to 3. Nothing can move yet.
  pager.NotifyPagesInserted(0, 3);
  DALI_TEST_EQUALS(pager.GetPageCount(), 4, TEST_LOCATION);
  DALI_TEST_EQUALS(pager.GetCurrentPage(), 3, TEST_LOCATION);
  DALI_TEST_EQUALS(pager.GetScrollPosition().x, 0.0f, 0.001f, TEST_LOCATION);

  window.Add(pager);
  application.SendNotification();
  application.Render();

  // Page 3 of the five real pages: scroll position 300, and the selection survived the
  // layout-derived count change that is reported alongside it.
  DALI_TEST_EQUALS(pager.GetScrollPosition().x, 300.0f, 0.001f, TEST_LOCATION);
  DALI_TEST_EQUALS(pager.GetCurrentPage(), 3, TEST_LOCATION);
  DALI_TEST_EQUALS(pager.GetPageCount(), 5, TEST_LOCATION);
  DALI_TEST_CHECK(!recorder.signals.empty());
  DALI_TEST_EQUALS(recorder.signals.back().first, 3, TEST_LOCATION);
  DALI_TEST_EQUALS(recorder.signals.back().second, 5, TEST_LOCATION);
  END_TEST;
}
