/* Copyright (c) 2026 Samsung Electronics Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */
#include "layout-validation-support.h"

using namespace LayoutValidation;
namespace
{
struct Mixed
{
  StackLayout root;
  GridLayout grid;
  FlexLayout flex;
  AbsoluteLayout absolute;
  std::vector<View> cells;
  std::vector<View> flexItems;
  View proportional;
  View negative;
};
std::shared_ptr<Mixed> MakeMixed()
{
  auto s = std::make_shared<Mixed>();
  s->root = StackLayout::New(StackOrientation::VERTICAL);
  s->root.SetRequestedHeight(WRAP_CONTENT); s->root.SetPadding(Insets(7, 11, 5, 9)); s->root.SetSpacing(13);
  s->grid = GridLayout::New(); s->grid.SetRequestedWidth(MATCH_PARENT); s->grid.SetRequestedHeight(90);
  s->grid.SetColumnSpacing(4); s->grid.SetRowSpacing(3);
  s->grid.AddColumnDefinition(GridLength::Absolute(40)); s->grid.AddColumnDefinition(GridLength::Star(1)); s->grid.AddColumnDefinition(GridLength::Star(2));
  s->grid.AddRowDefinition(GridLength::Absolute(20)); s->grid.AddRowDefinition(GridLength::Star(1));
  for(uint32_t i = 0; i < 6; ++i)
  {
    auto cell = Leaf(MATCH_PARENT, MATCH_PARENT); cell.SetLayoutParams(GridLayoutParams::New().SetRow(i / 3).SetColumn(i % 3));
    s->grid.Add(cell); s->cells.push_back(cell);
  }
  s->flex = FlexLayout::New(); s->flex.SetDirection(FlexDirection::ROW); s->flex.SetWrap(FlexWrap::NO_WRAP);
  s->flex.SetJustifyContent(FlexJustify::SPACE_BETWEEN); s->flex.SetAlignItems(FlexAlign::CENTER);
  s->flex.SetRequestedWidth(MATCH_PARENT); s->flex.SetRequestedHeight(60);
  for(uint32_t i = 0; i < 3; ++i)
  {
    auto item = Leaf(20 + 10 * i, 10); item.SetLayoutParams(FlexLayoutParams::New().SetFlexShrink(0));
    s->flex.Add(item); s->flexItems.push_back(item);
  }
  s->absolute = AbsoluteLayout::New(); s->absolute.SetRequestedWidth(MATCH_PARENT); s->absolute.SetRequestedHeight(70);
  s->proportional = Leaf(20, 10); s->proportional.SetLayoutParams(AbsoluteLayoutParams::New().SetBounds(LayoutRect(.5f, 1, 20, 10)).SetFlags(AbsoluteLayoutFlags::POSITION_PROPORTIONAL));
  s->negative = Leaf(30, 12); s->negative.SetLayoutParams(AbsoluteLayoutParams::New().SetBounds(LayoutRect(-7, 4, 30, 12)));
  s->absolute.Add(s->proportional); s->absolute.Add(s->negative);
  s->root.Add(s->grid); s->root.Add(s->flex); s->root.Add(s->absolute);
  return s;
}
void CheckMixed(Run& r, Mixed& s, float width, const std::string& prefix = "")
{
  const float c = width - 18;
  r.Size((prefix + "root.measure").c_str(), s.root.Measure(width, 600), MeasuredSize(width, 260));
  r.Rect((prefix + "root.arrange").c_str(), s.root.Arrange(LayoutRect(5, 7, width, 260)), LayoutRect(5, 7, width, 260));
  r.Rect((prefix + "grid").c_str(), s.grid, LayoutRect(7, 5, c, 90));
  r.Rect((prefix + "flex").c_str(), s.flex, LayoutRect(7, 108, c, 60));
  r.Rect((prefix + "absolute").c_str(), s.absolute, LayoutRect(7, 181, c, 70));
  const float unit = (c - 48) / 3;
  const float xs[] = {0, 44, 48 + unit}; const float widths[] = {40, unit, 2 * unit};
  for(uint32_t i = 0; i < 6; ++i) r.Rect((prefix + Id("cell", i)).c_str(), s.cells[i], LayoutRect(xs[i % 3], i < 3 ? 0 : 23, widths[i % 3], i < 3 ? 20 : 67));
  const float gap = (c - 90) / 2;
  r.Rect((prefix + "flex.0").c_str(), s.flexItems[0], LayoutRect(0, 25, 20, 10));
  r.Rect((prefix + "flex.1").c_str(), s.flexItems[1], LayoutRect(20 + gap, 25, 30, 10));
  r.Rect((prefix + "flex.2").c_str(), s.flexItems[2], LayoutRect(c - 40, 25, 40, 10));
  r.Rect((prefix + "absolute.proportional").c_str(), s.proportional, LayoutRect((c - 20) * .5f, 60, 20, 10));
  r.Rect((prefix + "absolute.negative").c_str(), s.negative, LayoutRect(-7, 4, 30, 12));
}
void CheckWindowMixed(Run& r, Mixed& s, float width)
{
  const float c=width-18;
  auto get=[&r](View v){return r.Snapshot(v);};
  r.Rect("root.window",get(s.root),LayoutRect(0,0,width,260));
  r.Rect("grid", get(s.grid), LayoutRect(7, 5, c, 90));
  r.Rect("flex", get(s.flex), LayoutRect(7, 108, c, 60));
  r.Rect("absolute", get(s.absolute), LayoutRect(7, 181, c, 70));
  const float unit = (c - 48) / 3;
  const float xs[] = {0, 44, 48 + unit}; const float widths[] = {40, unit, 2 * unit};
  for(uint32_t i = 0; i < 6; ++i) r.Rect(Id("cell", i).c_str(), get(s.cells[i]), LayoutRect(xs[i % 3], i < 3 ? 0 : 23, widths[i % 3], i < 3 ? 20 : 67));
  const float gap = (c - 90) / 2;
  r.Rect("flex.0", get(s.flexItems[0]), LayoutRect(0, 25, 20, 10));
  r.Rect("flex.1", get(s.flexItems[1]), LayoutRect(20 + gap, 25, 30, 10));
  r.Rect("flex.2", get(s.flexItems[2]), LayoutRect(c - 40, 25, 40, 10));
  r.Rect("absolute.proportional", get(s.proportional), LayoutRect((c - 20) * .5f, 60, 20, 10));
  r.Rect("absolute.negative", get(s.negative), LayoutRect(-7, 4, 30, 12));
}
class TcLr51 : public Case
{
public:
  TcLr51() : Case("LR51", "Mixed resize") {}
  std::vector<Scenario> BuildScenarios() override
  {
    std::vector<Scenario> out={{"mixed-roundtrip", "Independent nested track, line and proportional bounds across A-B-C-A widths", {
      {"width-160", 62, [](Run& r) { auto s = MakeMixed(); r.SetState(s); s->root.SetRequestedWidth(160); CheckMixed(r, *s, 160); }},
      {"width-263", 62, [](Run& r) { auto s = r.State<Mixed>(); s->root.SetRequestedWidth(263); CheckMixed(r, *s, 263); }},
      {"width-512", 62, [](Run& r) { auto s = r.State<Mixed>(); s->root.SetRequestedWidth(512); CheckMixed(r, *s, 512); }},
      {"return-160", 124, [](Run& r) { auto s = r.State<Mixed>(); s->root.SetRequestedWidth(160); CheckMixed(r, *s, 160, "existing."); auto fresh = MakeMixed(); fresh->root.SetRequestedWidth(160); CheckMixed(r, *fresh, 160, "fresh."); }}
    }}};
    Scenario window{"window-roundtrip","Actual Window snapshots across four nested layout managers",{}};
    for(float width : {160.f,263.f,512.f,160.f})
    {
      const bool first=window.steps.empty();
      window.steps.push_back({first?"mount-160":("resize-"+std::to_string(static_cast<int>(width))),60,[width,first](Run& r)
      {
        auto s=first?MakeMixed():r.State<Mixed>();if(first)r.SetState(s);
        std::vector<View> nodes{s->root,s->grid,s->flex,s->absolute};
        nodes.insert(nodes.end(),s->cells.begin(),s->cells.end());nodes.insert(nodes.end(),s->flexItems.begin(),s->flexItems.end());nodes.push_back(s->proportional);nodes.push_back(s->negative);
        r.AfterLayout(nodes,[s,width](Run& done){CheckWindowMixed(done,*s,width);});
        s->root.SetRequestedWidth(width);if(first)r.Attach(s->root);
      }});
    }
    out.push_back(std::move(window));return out;
  }
};
}
REGISTER_MANUAL_TEST(TcLr51)
