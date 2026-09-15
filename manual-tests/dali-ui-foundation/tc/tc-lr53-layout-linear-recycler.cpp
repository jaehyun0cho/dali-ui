/* Copyright (c) 2026 Samsung Electronics Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */
#include "layout-validation-recycler-fixtures.h"
using namespace LayoutValidation;
namespace
{
class TcLr53 : public Case
{
public:
  TcLr53() : Case("LR53", "Linear item layout") {}
  std::vector<Scenario> BuildScenarios() override
  {
    std::vector<Scenario> result;
    for(bool horizontal : {false, true})
      result.push_back(Single(horizontal ? "horizontal" : "vertical", "Fixed list extent, both scroll boundaries and wrong-axis no-op", 35, [horizontal](Run& r)
      {
        FixedRecycler fixture(10, 20, horizontal);
        auto layouter = LinearItemsLayouter::New(horizontal ? ItemsLayouter::Orientation::HORIZONTAL : ItemsLayouter::Orientation::VERTICAL);
        layouter.SetItemExtent(20); layouter.SetItemSpacing(2);
        auto& impl = layouter.GetImpl(); impl.OnLayoutChildren(fixture);
        r.Near("range", layouter.ComputeScrollRange(), 218); r.Near("extent", layouter.ComputeScrollExtent(), 50);
        r.Equal("first", layouter.GetFirstVisiblePosition(), 0); r.Equal("last", layouter.GetLastVisiblePosition(), 2);
        for(uint32_t i = 0; i < 3; ++i) r.Rect(Id("initial", i).c_str(), fixture.items[i], horizontal ? LayoutRect(22.f * i, 0, 20, 120) : LayoutRect(0, 22.f * i, 120, 20));
        auto scroll = [&](float delta) { return horizontal ? impl.ScrollHorizontallyBy(delta, fixture) : impl.ScrollVerticallyBy(delta, fixture); };
        r.Near("forward.consumed", scroll(26), 26); r.Near("forward.offset", layouter.ComputeScrollOffset(), 26);
        r.Equal("forward.first", layouter.GetFirstVisiblePosition(), 1); r.Equal("forward.last", layouter.GetLastVisiblePosition(), 3);
        r.Near("end.consumed", scroll(10000), 142); r.Near("end.offset", layouter.ComputeScrollOffset(), 168);
        r.Equal("end.first", layouter.GetFirstVisiblePosition(), 7); r.Equal("end.last", layouter.GetLastVisiblePosition(), 9);
        r.Near("end.clamp", scroll(1), 0); r.Near("start.consumed", scroll(-10000), -168); r.Near("start.offset", layouter.ComputeScrollOffset(), 0);
        r.Near("wrong.axis", horizontal ? impl.ScrollVerticallyBy(12, fixture) : impl.ScrollHorizontallyBy(12, fixture), 0);
        r.Truth("recycling.used", fixture.recycled > 0);
        r.Rect("item.bounds", layouter.GetItemBounds(4, 120), horizontal ? LayoutRect(88, 0, 20, 120) : LayoutRect(0, 88, 120, 20));
        r.Equal("active.range", layouter.GetLastVisiblePosition(), 2);
        r.Truth("axis.capability", horizontal ? layouter.CanScrollHorizontally() && !layouter.CanScrollVertically() : layouter.CanScrollVertically() && !layouter.CanScrollHorizontally());
      }));
    result.push_back(Single("cache-window", "Cache extents expand the active range independently of the viewport", 8, [](Run& r)
    {
      FixedRecycler fixture(10); fixture.before = 22; fixture.after = 22;
      auto layouter = LinearItemsLayouter::New(); layouter.SetItemExtent(20); layouter.SetItemSpacing(2);
      auto& impl = layouter.GetImpl(); impl.OnLayoutChildren(fixture);
      r.Equal("cache.first", layouter.GetFirstVisiblePosition(), 0); r.Equal("cache.last", layouter.GetLastVisiblePosition(), 3);
      r.Near("cache.scroll", impl.ScrollVerticallyBy(44, fixture), 44);
      r.Equal("cache.scrolled.first", layouter.GetFirstVisiblePosition(), 1); r.Equal("cache.scrolled.last", layouter.GetLastVisiblePosition(), 5);
      r.Near("cache.range", layouter.ComputeScrollRange(), 218); r.Near("cache.viewport", layouter.ComputeScrollExtent(), 50);
      r.Truth("cache.materialization", fixture.active[1] && fixture.active[5]);
    }));
    result.push_back(Single("variable-decoration", "Measured sizes and decoration offsets determine every content-space slot", 22, [](Run& r)
    {
      FixedRecycler fixture(3); fixture.viewport = 200;
      fixture.items[0].SetRequestedHeight(10); fixture.items[1].SetRequestedHeight(30); fixture.items[2].SetRequestedHeight(20);
      fixture.offsets.left = 3; fixture.offsets.right = 7; fixture.offsets.top = 2; fixture.offsets.bottom = 4;
      auto layouter = LinearItemsLayouter::New(); layouter.SetItemExtent(20); layouter.SetItemSpacing(5); layouter.GetImpl().OnLayoutChildren(fixture);
      const float y[] = {2, 23, 64}; const float h[] = {10, 30, 20};
      for(uint32_t i = 0; i < 3; ++i) r.Rect(Id("decorated", i).c_str(), fixture.items[i], LayoutRect(3, y[i], 110, h[i]));
      r.Near("variable.range", layouter.ComputeScrollRange(), 88);
      r.Rect("variable.content_slot", layouter.GetItemBounds(1, 120), LayoutRect(0, 21, 120, 36));
      layouter.SetItemExtent(-5); r.Near("extent.clamp", layouter.GetItemExtent(), 1);
      layouter.SetItemSpacing(-3); r.Near("spacing.clamp", layouter.GetItemSpacing(), 0);
      layouter.SetOrientation(ItemsLayouter::Orientation::HORIZONTAL);
      r.Near("orientation.offset.reset", layouter.ComputeScrollOffset(), 0);
      r.Equal("orientation.first.reset", layouter.GetFirstVisiblePosition(), 0); r.Equal("orientation.last.reset", layouter.GetLastVisiblePosition(), 0);
    }));
    result.push_back(Single("empty-zero-viewport", "Empty data and zero viewport perform no materialization", 6, [](Run& r)
    {
      FixedRecycler empty(0); auto layouter = LinearItemsLayouter::New(); layouter.GetImpl().OnLayoutChildren(empty);
      r.Near("empty.range", layouter.ComputeScrollRange(), 0); r.Equal("empty.gets", empty.gets, 0); r.Near("empty.scroll", layouter.GetImpl().ScrollVerticallyBy(10, empty), 0);
      FixedRecycler zero(10); zero.viewport = 0; layouter.GetImpl().OnLayoutChildren(zero);
      r.Equal("zero.gets", zero.gets, 0); r.Near("zero.extent", layouter.ComputeScrollExtent(), 0); r.Near("zero.scroll", layouter.GetImpl().ScrollVerticallyBy(10, zero), 0);
    }));
    return result;
  }
};
}
REGISTER_MANUAL_TEST(TcLr53)
