/* Copyright (c) 2026 Samsung Electronics Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */
#include <dali/devel-api/text-abstraction/font-client.h>
#include <filesystem>
#include "layout-validation-recycler-fixtures.h"
#include "layout-validation-workloads.h"
using namespace LayoutValidation;
namespace
{
enum class Family
{
  DEEP,
  BALANCED,
  FLEX_WRAP,
  FLEX_GROW,
  FLEX_SHRINK,
  GRID_SPAN,
  RECYCLER,
  TEXT
};
const char* Name(Family f)
{
  switch(f)
  {
    case Family::DEEP:
      return "deep-dirty";
    case Family::BALANCED:
      return "balanced-dirty";
    case Family::FLEX_WRAP:
      return "flex-wrap";
    case Family::FLEX_GROW:
      return "flex-grow";
    case Family::FLEX_SHRINK:
      return "flex-shrink";
    case Family::GRID_SPAN:
      return "grid-star-span";
    case Family::RECYCLER:
      return "variable-recycler";
    case Family::TEXT:
      return "text-reflow";
  }
  return "invalid";
}
struct TreeNode
{
  View  view;
  float y{0}, height{20};
  bool  changedBranch{false};
};
struct ComplexFixture
{
  Family                         family;
  uint32_t                       count;
  Workload                       work;
  std::vector<TreeNode>          tree;
  View                           dirtyLeaf;
  std::unique_ptr<FixedRecycler> recycler;
  LinearItemsLayouter            layouter;
  Label                          text;
  float                          range{0};
  uint32_t                       columns{1};
  MeasuredSize                   measured;
  LayoutRect                     arranged;

  explicit ComplexFixture(Family f, uint32_t n)
  : family(f),
    count(n)
  {
    if(f == Family::DEEP || f == Family::BALANCED)
    {
      work.width  = 120;
      auto node   = MakeTree(f == Family::DEEP ? n : (n == 16 ? 4u : 7u), true);
      work.root   = node.view;
      work.height = node.height;
      // The root is covered separately; every descendant, including containers, is observed.
      tree.pop_back();
      for(const auto& entry : tree)
      {
        work.leaves.push_back(entry.view);
        work.expected.emplace_back(0, entry.y, 120, entry.height);
      }
    }
    else if(f == Family::FLEX_WRAP || f == Family::FLEX_GROW || f == Family::FLEX_SHRINK)
    {
      auto flex = FlexLayout::New();
      flex.SetDirection(FlexDirection::ROW);
      flex.SetWrap(f == Family::FLEX_WRAP ? FlexWrap::WRAP : FlexWrap::NO_WRAP);
      flex.SetAlignItems(FlexAlign::FLEX_START);
      flex.SetAlignContent(FlexAlign::FLEX_START);
      flex.SetJustifyContent(FlexJustify::FLEX_START);
      work.root   = flex;
      work.height = f == Family::FLEX_WRAP ? n * 16.f : 16.f;
      flex.SetRequestedHeight(work.height);
      for(uint32_t i = 0; i < n; ++i)
      {
        auto leaf = Leaf(f == Family::FLEX_WRAP ? 10 : f == Family::FLEX_GROW ? 8
                                                                              : 16,
                         16);
        const float weight = i % 2 ? 3.f : 1.f;
        leaf.SetLayoutParams(FlexLayoutParams::New().SetFlexBasis(f == Family::FLEX_WRAP ? 10 : f == Family::FLEX_GROW ? 8
                                                                                                                       : 16)
                               .SetFlexGrow(f == Family::FLEX_GROW ? weight : 0)
                               .SetFlexShrink(f == Family::FLEX_SHRINK ? weight : 0));
        work.root.Add(leaf);
        work.leaves.push_back(leaf);
        work.expected.emplace_back();
      }
    }
    else if(f == Family::GRID_SPAN)
    {
      auto grid   = GridLayout::New();
      work.root   = grid;
      work.height = 20.f * n;
      grid.SetRequestedHeight(work.height);
      for(float star : {1.f, 2.f, 1.f, 1.f}) grid.AddColumnDefinition(GridLength::Star(star));
      for(uint32_t row = 0; row < n; ++row)
      {
        grid.AddRowDefinition(GridLength::Star(1));
        for(uint32_t column : {0u, 2u})
        {
          auto leaf = Leaf(8, 8);
          leaf.SetLayoutParams(GridLayoutParams::New().SetRow(row).SetColumn(column).SetColumnSpan(column ? 1 : 2));
          grid.Add(leaf);
          work.leaves.push_back(leaf);
          work.expected.emplace_back();
        }
      }
      auto spanning = Leaf(8, 8);
      spanning.SetLayoutParams(GridLayoutParams::New().SetColumn(3).SetRowSpan(n));
      grid.Add(spanning);
      work.leaves.push_back(spanning);
      work.expected.emplace_back();
    }
    else if(f == Family::RECYCLER)
    {
      recycler                 = std::make_unique<FixedRecycler>(n);
      recycler->viewport       = 100000;
      recycler->offsets.left   = 3;
      recycler->offsets.right  = 7;
      recycler->offsets.top    = 2;
      recycler->offsets.bottom = 4;
      layouter                 = LinearItemsLayouter::New();
      layouter.SetItemExtent(20);
      layouter.SetItemSpacing(5);
      for(uint32_t i = 0; i < n; ++i)
      {
        recycler->items[i].SetRequestedHeight(10.f * (1 + i % 3));
        work.leaves.push_back(recycler->items[i]);
        work.expected.emplace_back();
      }
    }
    else
    {
      text = Label::New(std::string(n, 'A').c_str());
      text.SetFontFamily("DejaVu Sans");
      text.SetFontSize(24);
      text.SetMultiLine(true);
      text.SetLineWrapMode(Text::LineWrapMode::CHARACTER);
      text.SetRequestedHeight(WRAP_CONTENT);
      work.root = text;
    }
    Prepare(0);
    Mutate(0);
    Calculate();
  }
  TreeNode MakeTree(uint32_t depth, bool rightmost)
  {
    if(depth == 0)
    {
      auto leaf = Leaf(120, 20);
      if(rightmost) dirtyLeaf = leaf;
      TreeNode result{leaf, 0, 20, rightmost};
      tree.push_back(result);
      return result;
    }
    auto root = StackLayout::New(StackOrientation::VERTICAL);
    root.SetRequestedWidth(120);
    root.SetRequestedHeight(WRAP_CONTENT);
    root.SetSpacing(family == Family::BALANCED ? 2 : 0);
    auto first = MakeTree(depth - 1, family == Family::DEEP && rightmost);
    root.Add(first.view);
    float height = first.height;
    if(family == Family::BALANCED)
    {
      auto second = MakeTree(depth - 1, rightmost);
      root.Add(second.view);
      tree.back().y = first.height + 2;
      height += second.height + 2;
    }
    TreeNode result{root, 0, height, rightmost};
    tree.push_back(result);
    return result;
  }
  void Prepare(uint32_t state)
  {
    if(family == Family::DEEP || family == Family::BALANCED)
    {
      work.height = family == Family::DEEP ? 20.f + state : count * 22.f - 2 + state;
      for(std::size_t i = 0; i < tree.size(); ++i) work.expected[i] = {0, tree[i].y, 120, tree[i].height + (tree[i].changedBranch ? state : 0)};
    }
    else if(family == Family::FLEX_WRAP)
    {
      columns    = state ? 5 : 4;
      work.width = 10.f * columns;
      for(uint32_t i = 0; i < count; ++i) work.expected[i] = {10.f * (i % columns), 16.f * (i / columns), 10, 16};
    }
    else if(family == Family::FLEX_GROW || family == Family::FLEX_SHRINK)
    {
      work.width = (family == Family::FLEX_GROW ? (state ? 16.f : 12.f) : (state ? 8.f : 12.f)) * count;
      float x    = 0;
      for(uint32_t i = 0; i < count; ++i)
      {
        const float weight = i % 2 ? 3.f : 1.f;
        const float width  = family == Family::FLEX_GROW ? 8 + (state ? 4.f : 2.f) * weight : 16 - (state ? 4.f : 2.f) * weight;
        work.expected[i]   = {x, 0, width, 16};
        x += width;
      }
    }
    else if(family == Family::GRID_SPAN)
    {
      work.width       = state ? 375 : 250;
      const float unit = work.width / 5;
      for(uint32_t i = 0; i < count; ++i)
      {
        work.expected[2 * i]     = {0, 20.f * i, 3 * unit, 20};
        work.expected[2 * i + 1] = {3 * unit, 20.f * i, unit, 20};
      }
      work.expected.back() = {4 * unit, 0, unit, work.height};
    }
    else if(family == Family::RECYCLER)
    {
      work.width = state ? 160 : 120;
      float y    = 0;
      for(uint32_t i = 0; i < count; ++i)
      {
        const float h    = 10.f * (1 + i % 3);
        work.expected[i] = {3, y + 2, work.width - 10, h};
        y += h + 6 + 5;
      }
      range = y - 5;
    }
    else
    {
      work.width = state ? 40 : 25;
      columns    = state ? 2 : 1;
      // Locked DejaVu Sans 24px: A advance ~16px; hinted ascender23 + descender6 = 29px.
      work.height = 29.f * ((count + columns - 1) / columns);
    }
  }
  void Mutate(uint32_t state)
  {
    if(family == Family::DEEP || family == Family::BALANCED)
      dirtyLeaf.SetRequestedHeight(20.f + state);
    else if(family == Family::RECYCLER)
      recycler->cross = work.width;
    else
      work.root.SetRequestedWidth(work.width);
  }
  void Calculate()
  {
    if(family == Family::RECYCLER)
      layouter.GetImpl().OnLayoutChildren(*recycler);
    else
    {
      measured = work.root.Measure(work.width, 100000);
      arranged = work.root.Arrange({0, 0, work.width, work.height});
    }
  }
  ImmediateWorkResult Observe() const
  {
    ImmediateWorkResult result;
    if(family == Family::RECYCLER)
    {
      result.Add(layouter.ComputeScrollRange(), range);
      result.Add(layouter.ComputeScrollOffset(), 0);
      result.Add(static_cast<float>(layouter.GetFirstVisiblePosition()), 0);
      result.Add(static_cast<float>(layouter.GetLastVisiblePosition()), count - 1.f);
      for(uint32_t i = 0; i < count; ++i)
      {
        result.Add(LayoutValidation::Bounds(work.leaves[i]), work.expected[i]);
        result.Add(layouter.GetItemBounds(i, work.width), {0, work.expected[i].y - 2, work.width, work.expected[i].height + 6});
      }
    }
    else
    {
      result.Add(measured.width, work.width);
      result.Add(measured.height, work.height);
      result.Add(arranged, {0, 0, work.width, work.height});
      ObserveArrangedGeometry(result, work);
    }
    return result;
  }
  uint64_t Values() const
  {
    return family == Family::RECYCLER ? 4 + 8ull * count : 10 + 4ull * work.leaves.size();
  }
};
void BenchmarkComplex(Run& r, Family family, uint32_t count)
{
#if defined(DEBUG_ENABLED) && !defined(DALI_UI_LAYOUT_TEST_DIAGNOSTICS)
  r.Fail("profile.release", "timing requires Release OFF app and library");
#else
  ComplexFixture    fixture(family, count);
  const std::string metric = std::string("LR64.") + Name(family) + ".n" + std::to_string(count) + ".update";
  for(uint32_t sample = 0; sample < 41; ++sample)
  {
    const uint32_t state = (sample + 1) % 2;
    fixture.Prepare(state); // Independent expected-value construction is outside the measured interval.
    const auto start = std::chrono::steady_clock::now();
    fixture.Mutate(state);
    fixture.Calculate();
    const double elapsed  = std::chrono::duration<double, std::nano>(std::chrono::steady_clock::now() - start).count();
    const auto   observed = fixture.Observe(); // No repair Measure/Arrange before this observation.
    if(sample < 10) continue;
    observed.Check(r, Id("sample.immediate", sample - 10).c_str(), fixture.Values());
#if !defined(DALI_UI_LAYOUT_TEST_DIAGNOSTICS)
    r.Truth(Id("sample.timer", sample - 10).c_str(), std::isfinite(elapsed) && elapsed > 0);
    r.Sample(metric.c_str(), sample - 10, elapsed, 1);
#else
    static_cast<void>(elapsed);
#endif
    if(sample == 40)
    {
      if(family != Family::RECYCLER) r.Rect("final.root", fixture.work.root, {0, 0, fixture.work.width, fixture.work.height});
      for(uint32_t i = 0; i < fixture.work.leaves.size(); ++i) r.Rect(Id("final.node", i).c_str(), fixture.work.leaves[i], fixture.work.expected[i]);
    }
  }
  r.RequireExternalVerification("LR64 requires pinned fixture/font fingerprints and matching Release OFF paired reference/candidate comparison; ON timing is excluded");
#endif
}
std::size_t DetailRows(Family f, uint32_t n)
{
  if(f == Family::DEEP) return 4ull * (n + 1);
  if(f == Family::BALANCED) return 4ull * (2 * n - 1);
  if(f == Family::GRID_SPAN) return 4ull * (2 * n + 2);
  if(f == Family::RECYCLER) return 4ull * n;
  if(f == Family::TEXT) return 4;
  return 4ull * (n + 1);
}
class TcLr64 : public Case
{
public:
  TcLr64()
  : Case("LR64", "Complex layout performance")
  {
  }
  std::vector<Scenario> BuildScenarios() override
  {
    std::vector<Scenario> result;
    for(auto family : {Family::DEEP, Family::BALANCED, Family::FLEX_WRAP, Family::FLEX_GROW, Family::FLEX_SHRINK, Family::GRID_SPAN, Family::RECYCLER, Family::TEXT})
      for(uint32_t count : {16u, 128u})
      {
        Scenario scenario{std::string(Name(family)) + "-n" + std::to_string(count), "Immediate independent geometry and matched compound workload timing", {}};
#if defined(DALI_UI_LAYOUT_TEST_DIAGNOSTICS)
        if(family == Family::TEXT) scenario.steps.push_back({"font-identity", 5, [](Run& r)
        {
          auto label = Label::New("A");
          label.SetFontFamily("DejaVu Sans");
          label.SetFontSize(24);
          label.SetRequestedWidth(100);
          label.SetRequestedHeight(60);
          r.AfterLayout({label}, [label](Run& done)
          {
            auto t = WorkDiagnostic::GetTextSnapshot(label);
            done.Truth("font.model.ready", t.valid && t.ready);
            done.Equal("font.glyph.A", t.firstGlyphIndex, 36);
            Dali::TextAbstraction::FontDescription description;
            Dali::TextAbstraction::FontClient::Get().GetDescription(t.firstGlyphFontId, description);
            done.Text("font.path", std::filesystem::weakly_canonical(description.path).string(), std::filesystem::weakly_canonical(TEST_RESOURCE_DIR "/layout-validation/DejaVuSans.ttf").string());
            done.Near("font.ascender", t.firstAscender, 23, 1);
            done.Near("font.descender", t.firstDescender, -6, 1);
          });
          r.Attach(label);
        }});
        scenario.steps.push_back({"workload-correctness", 93 + DetailRows(family, count), [family, count](Run& r)
        { BenchmarkComplex(r, family, count); }});
#else
        scenario.steps.push_back({"timing", 124 + DetailRows(family, count), [family, count](Run& r)
        { BenchmarkComplex(r, family, count); }});
#endif
        result.push_back(std::move(scenario));
      }
    return result;
  }
};
} //namespace
REGISTER_MANUAL_TEST(TcLr64)
