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

#include <map>
#include <vector>

#include <dali-ui-foundation/integration-api/items-layouter-impl.h>
#include <dali-ui-foundation/integration-api/recycler.h>
#include <dali-ui-foundation/public-api/views/recycler/linear-items-layouter.h>
#include <dali-ui-test-suite-utils.h>

using namespace Dali;
using namespace Dali::Ui;

namespace
{
using Orientation = ItemsLayouter::Orientation;

// Implements the existing recycler bridge. The layouter owns all measurement
// and placement decisions; this fixture supplies item content and records the
// positions requested through the bridge.
class LayoutRecycler : public Recycler
{
public:
  explicit LayoutRecycler(Orientation axis)
  : orientation(axis)
  {
  }

  View GetViewForPosition(uint32_t position) override
  {
    requested.push_back(position);
    if(position >= itemCount)
    {
      return View();
    }

    auto found = active.find(position);
    if(found == active.end())
    {
      View view = View::New();
      view.SetUiScalePolicy(UiScalePolicy::DISABLED);
      found = active.emplace(position, view).first;
    }

    const float extent = fractional ? 1.0f + static_cast<float>(position % 7u) * 0.25f : itemExtent;
    if(orientation == Orientation::VERTICAL)
    {
      found->second.SetRequestedWidth(MATCH_PARENT);
      found->second.SetRequestedHeight(extent);
    }
    else
    {
      found->second.SetRequestedWidth(extent);
      found->second.SetRequestedHeight(MATCH_PARENT);
    }
    return found->second;
  }

  void RecycleViewForPosition(uint32_t position) override
  {
    active.erase(position);
  }

  void RecycleAllViews() override
  {
    active.clear();
  }

  uint32_t GetItemCount() const override
  {
    return itemCount;
  }
  float GetViewportExtent() const override
  {
    return viewport;
  }
  float GetCrossExtent() const override
  {
    return cross;
  }
  float GetCacheBefore() const override
  {
    return 0.0f;
  }
  float GetCacheAfter() const override
  {
    return 0.0f;
  }

  ItemOffsets GetDecorationOffsets(uint32_t) const override
  {
    return decoration;
  }

  Orientation              orientation;
  uint32_t                 itemCount{0u};
  float                    viewport{10.0f};
  float                    cross{64.0f};
  float                    itemExtent{10.0f};
  bool                     fractional{false};
  ItemOffsets              decoration;
  std::vector<uint32_t>    requested;
  std::map<uint32_t, View> active;
};

bool SameRect(const LayoutRect& actual, float x, float y, float width, float height)
{
  return actual.x == x && actual.y == y && actual.width == width && actual.height == height;
}

LayoutRect ReadItemGeometry(View view)
{
  return LayoutRect(view.GetProperty<float>(Actor::Property::POSITION_X),
                    view.GetProperty<float>(Actor::Property::POSITION_Y),
                    view.GetProperty<float>(Actor::Property::SIZE_WIDTH),
                    view.GetProperty<float>(Actor::Property::SIZE_HEIGHT));
}

bool RequestedOnly(const LayoutRecycler& recycler, uint32_t position)
{
  return recycler.requested.size() == 1u && recycler.requested.front() == position;
}
} // namespace

void utc_dali_linear_items_layouter_storage_internal_startup(void)
{
  test_return_value = TET_UNDEF;
}

void utc_dali_linear_items_layouter_storage_internal_cleanup(void)
{
  test_return_value = TET_PASS;
}

int UtcDaliLinearItemsLayouterStorageSequentialGeometryP(void)
{
  UiTestApplication application;
  const Orientation axes[] = {Orientation::VERTICAL, Orientation::HORIZONTAL};
  for(Orientation axis : axes)
  {
    LinearItemsLayouter layouter = LinearItemsLayouter::New(axis);
    LayoutRecycler      recycler(axis);
    recycler.itemCount  = 256u;
    recycler.viewport   = 10000.0f;
    recycler.fractional = true;
    layouter.SetItemExtent(1.0f);
    layouter.SetItemSpacing(0.5f);
    layouter.GetImpl().OnLayoutChildren(recycler);

    DALI_TEST_CHECK(recycler.requested.size() == 256u);
    DALI_TEST_CHECK(recycler.active.size() == 256u);
    DALI_TEST_CHECK(layouter.GetFirstVisiblePosition() == 0u);
    DALI_TEST_CHECK(layouter.GetLastVisiblePosition() == 255u);
    DALI_TEST_CHECK(layouter.ComputeScrollRange() == 574.0f);
    float offset = 0.0f;
    for(uint32_t i = 0u; i < 256u; ++i)
    {
      DALI_TEST_CHECK(recycler.requested.at(i) == i);
      const float      extent   = 1.0f + static_cast<float>(i % 7u) * 0.25f;
      const LayoutRect bounds   = layouter.GetItemBounds(i, recycler.cross);
      const LayoutRect geometry = ReadItemGeometry(recycler.active.at(i));
      if(axis == Orientation::VERTICAL)
      {
        DALI_TEST_CHECK(SameRect(bounds, 0.0f, offset, 64.0f, extent));
        DALI_TEST_CHECK(SameRect(geometry, 0.0f, offset, 64.0f, extent));
      }
      else
      {
        DALI_TEST_CHECK(SameRect(bounds, offset, 0.0f, extent, 64.0f));
        DALI_TEST_CHECK(SameRect(geometry, offset, 0.0f, extent, 64.0f));
      }
      offset += extent + 0.5f;
    }
    DALI_TEST_CHECK(offset - 0.5f == 574.0f);
  }
  END_TEST;
}

int UtcDaliLinearItemsLayouterStorageSparseRemeasureGeometryP(void)
{
  UiTestApplication   application;
  LinearItemsLayouter layouter = LinearItemsLayouter::New(Orientation::VERTICAL);
  LayoutRecycler      recycler(Orientation::VERTICAL);
  recycler.itemCount  = 10000u;
  recycler.itemExtent = 12.5f;
  layouter.SetItemExtent(10.0f);

  // Start through the cold scroll entry point, so the requested item is
  // measured without creating preceding items.
  const float consumed = layouter.GetImpl().ScrollVerticallyBy(40950.0f, recycler);
  DALI_TEST_CHECK(consumed == 40950.0f);
  DALI_TEST_CHECK(RequestedOnly(recycler, 4095u));
  DALI_TEST_CHECK(layouter.GetFirstVisiblePosition() == 4095u);
  DALI_TEST_CHECK(layouter.GetLastVisiblePosition() == 4095u);
  DALI_TEST_CHECK(layouter.ComputeScrollOffset() == 40950.0f);
  DALI_TEST_CHECK(layouter.ComputeScrollRange() == 125000.0f);
  DALI_TEST_CHECK(SameRect(layouter.GetItemBounds(4095u, 64.0f), 0.0f, 40950.0f, 64.0f, 12.5f));
  DALI_TEST_CHECK(SameRect(ReadItemGeometry(recycler.active.at(4095u)), 0.0f, 40950.0f, 64.0f, 12.5f));
  DALI_TEST_CHECK(SameRect(layouter.GetItemBounds(4094u, 64.0f), 0.0f, 40940.0f, 64.0f, 10.0f));
  DALI_TEST_CHECK(SameRect(layouter.GetItemBounds(4096u, 64.0f), 0.0f, 40962.5f, 64.0f, 10.0f));

  recycler.itemExtent = 15.75f;
  recycler.requested.clear();
  layouter.GetImpl().OnLayoutChildren(recycler);
  DALI_TEST_CHECK(RequestedOnly(recycler, 4095u));
  DALI_TEST_CHECK(layouter.ComputeScrollRange() == 157500.0f);
  DALI_TEST_CHECK(SameRect(layouter.GetItemBounds(4095u, 64.0f), 0.0f, 40950.0f, 64.0f, 15.75f));
  DALI_TEST_CHECK(SameRect(ReadItemGeometry(recycler.active.at(4095u)), 0.0f, 40950.0f, 64.0f, 15.75f));
  DALI_TEST_CHECK(SameRect(layouter.GetItemBounds(4096u, 64.0f), 0.0f, 40965.75f, 64.0f, 10.0f));
  END_TEST;
}

int UtcDaliLinearItemsLayouterStorageAdapterResetDiscardsMeasurementsP(void)
{
  UiTestApplication   application;
  LinearItemsLayouter layouter = LinearItemsLayouter::New(Orientation::VERTICAL);
  LayoutRecycler      recycler(Orientation::VERTICAL);
  recycler.itemCount  = 3u;
  recycler.viewport   = 100.0f;
  recycler.itemExtent = 12.5f;
  layouter.SetItemExtent(10.0f);
  layouter.GetImpl().OnLayoutChildren(recycler);
  DALI_TEST_CHECK(recycler.requested.size() == 3u);
  DALI_TEST_CHECK(layouter.ComputeScrollRange() == 37.5f);
  DALI_TEST_CHECK(SameRect(layouter.GetItemBounds(1u, 64.0f), 0.0f, 12.5f, 64.0f, 12.5f));

  layouter.GetImpl().OnAdapterChanged();
  DALI_TEST_CHECK(layouter.ComputeScrollOffset() == 0.0f);
  DALI_TEST_CHECK(layouter.ComputeScrollRange() == 0.0f);
  DALI_TEST_CHECK(SameRect(layouter.GetItemBounds(1u, 64.0f), 0.0f, 10.0f, 64.0f, 10.0f));

  // One new item fills this viewport even though it is shorter than the
  // estimate. Other positions stay unmeasured, exposing any stale sizes.
  recycler.viewport   = 5.0f;
  recycler.itemExtent = 7.0f;
  recycler.requested.clear();
  layouter.GetImpl().OnLayoutChildren(recycler);
  DALI_TEST_CHECK(RequestedOnly(recycler, 0u));
  DALI_TEST_CHECK(layouter.ComputeScrollRange() == 21.0f);
  DALI_TEST_CHECK(SameRect(ReadItemGeometry(recycler.active.at(0u)), 0.0f, 0.0f, 64.0f, 7.0f));
  DALI_TEST_CHECK(SameRect(layouter.GetItemBounds(1u, 64.0f), 0.0f, 7.0f, 64.0f, 10.0f));
  DALI_TEST_CHECK(SameRect(layouter.GetItemBounds(2u, 64.0f), 0.0f, 17.0f, 64.0f, 10.0f));
  END_TEST;
}

int UtcDaliLinearItemsLayouterStorageDecorationResetRecomputesSlotsP(void)
{
  UiTestApplication   application;
  LinearItemsLayouter layouter = LinearItemsLayouter::New(Orientation::VERTICAL);
  LayoutRecycler      recycler(Orientation::VERTICAL);
  recycler.itemCount  = 3u;
  recycler.viewport   = 100.0f;
  recycler.itemExtent = 7.0f;
  recycler.decoration = ItemOffsets{3.0f, 3.0f, 4.0f, 4.0f};
  layouter.SetItemExtent(10.0f);
  layouter.GetImpl().OnLayoutChildren(recycler);
  DALI_TEST_CHECK(recycler.requested.size() == 3u);
  DALI_TEST_CHECK(layouter.ComputeScrollRange() == 42.0f);
  DALI_TEST_CHECK(SameRect(layouter.GetItemBounds(0u, 64.0f), 0.0f, 0.0f, 64.0f, 14.0f));
  DALI_TEST_CHECK(SameRect(ReadItemGeometry(recycler.active.at(0u)), 3.0f, 3.0f, 57.0f, 7.0f));

  auto* decorationSupport = dynamic_cast<Dali::Ui::Integration::ItemsLayouterDecorationSupport*>(&layouter.GetImpl());
  DALI_TEST_CHECK(decorationSupport != nullptr);
  if(!decorationSupport)
  {
    END_TEST;
  }
  recycler.decoration = ItemOffsets{5.0f, 2.0f, 6.0f, 6.0f};
  decorationSupport->OnDecorationChanged();
  DALI_TEST_CHECK(layouter.ComputeScrollOffset() == 0.0f);
  DALI_TEST_CHECK(layouter.ComputeScrollRange() == 30.0f);
  DALI_TEST_CHECK(SameRect(layouter.GetItemBounds(0u, 64.0f), 0.0f, 0.0f, 64.0f, 10.0f));

  recycler.requested.clear();
  layouter.GetImpl().OnLayoutChildren(recycler);
  DALI_TEST_CHECK(recycler.requested.size() == 3u);
  for(uint32_t i = 0u; i < 3u; ++i)
  {
    DALI_TEST_CHECK(recycler.requested.at(i) == i);
    DALI_TEST_CHECK(SameRect(layouter.GetItemBounds(i, 64.0f), 0.0f, 15.0f * i, 64.0f, 15.0f));
    DALI_TEST_CHECK(SameRect(ReadItemGeometry(recycler.active.at(i)), 5.0f, 15.0f * i + 2.0f, 53.0f, 7.0f));
  }
  DALI_TEST_CHECK(layouter.ComputeScrollRange() == 45.0f);
  END_TEST;
}
