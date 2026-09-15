/* Copyright (c) 2026 Samsung Electronics Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy at http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 */
#include "layout-validation-fixtures.h"

using namespace LayoutValidation;
using namespace LayoutValidation::Fixtures;

class TcLr58 : public Case
{
public:
  TcLr58()
  : Case("LR58", "Deterministic seeds")
  {
  }
  std::vector<Scenario> BuildScenarios() override
  {
    std::vector<Scenario> out;
    for(uint32_t seed : {1u, 17u, 73u, 257u, 65537u}) out.push_back(Single(Id("R01.seed.", seed).c_str(), "Seeded stack conservation and axis metamorphism", 130, [seed](Run& r)
    {
      auto h = StackLayout::New(StackOrientation::HORIZONTAL), v = StackLayout::New(StackOrientation::VERTICAL);
      h.SetSpacing(3);
      v.SetSpacing(3);
      std::vector<View>  hs, vs;
      std::vector<float> widths, heights;
      uint32_t           state = seed;
      float              total = 0, maxCross = 0;
      for(unsigned i = 0; i < 16; ++i)
      {
        state    = state * 1664525u + 1013904223u;
        float w  = 1 + (state % 47);
        state    = state * 1664525u + 1013904223u;
        float he = 1 + (state % 31);
        widths.push_back(w);
        heights.push_back(he);
        hs.push_back(Leaf(w, he));
        vs.push_back(Leaf(he, w));
        h.Add(hs.back());
        v.Add(vs.back());
        total += w;
        maxCross = std::max(maxCross, he);
      }
      total += 45;
      r.Size("intrinsic", h.Measure(1000, 1000), MeasuredSize(total, maxCross));
      h.Arrange(LayoutRect(0, 0, total, maxCross));
      v.Measure(1000, 1000);
      v.Arrange(LayoutRect(0, 0, maxCross, total));
      float position = 0;
      for(unsigned i = 0; i < 16; ++i)
      {
        r.Rect(Id("horizontal.", i).c_str(), hs[i], LayoutRect(position, 0, widths[i], heights[i]));
        r.Rect(Id("vertical.", i).c_str(), vs[i], LayoutRect(0, position, heights[i], widths[i]));
        position += widths[i] + 3;
      }
    }));

    for(uint32_t seed : {1u, 73u, 65537u})
    {
      out.push_back(Single(Id("R02.flex-seed.", seed).c_str(), "Seeded grow conservation, reversal, RTL, translation and transpose", 160, [seed](Run& r)
      {
        uint32_t           state = seed;
        std::vector<float> widths, heights;
        std::vector<View>  children, transposed;
        float              total = 120;
        auto               f = FlexLayout::New(), v = FlexLayout::New();
        f.SetAlignItems(FlexAlign::FLEX_START);
        v.SetAlignItems(FlexAlign::FLEX_START);
        v.SetDirection(FlexDirection::COLUMN);
        for(unsigned i = 0; i < 8; ++i)
        {
          state   = state * 1664525u + 1013904223u;
          float w = 1 + state % 47;
          state   = state * 1664525u + 1013904223u;
          float h = 1 + state % 31;
          widths.push_back(w);
          heights.push_back(h);
          total += w;
          View a = Leaf(w, h), b = Leaf(h, w);
          a.SetLayoutParams(FlexLayoutParams::New().SetFlexBasis(w).SetFlexGrow(1));
          b.SetLayoutParams(FlexLayoutParams::New().SetFlexBasis(w).SetFlexGrow(1));
          f.Add(a);
          v.Add(b);
          children.push_back(a);
          transposed.push_back(b);
        }
        Fixed(f, total, 80);
        Fixed(v, 80, total);
        Compute(f, total, 80);
        float x = 0;
        for(unsigned i = 0; i < 8; ++i)
        {
          r.Rect(Id("flex.forward.", i).c_str(), children[i], LayoutRect(x, 0, widths[i] + 15, heights[i]));
          x += widths[i] + 15;
        }
        f.SetDirection(FlexDirection::ROW_REVERSE);
        Compute(f, total, 80);
        x = 0;
        for(unsigned i = 0; i < 8; ++i)
        {
          r.Rect(Id("flex.reverse.", i).c_str(), children[i], LayoutRect(total - x - widths[i] - 15, 0, widths[i] + 15, heights[i]));
          x += widths[i] + 15;
        }
        f.SetLayoutDirection(LayoutDirection::RIGHT_TO_LEFT);
        Compute(f, total, 80);
        x = 0;
        for(unsigned i = 0; i < 8; ++i)
        {
          r.Rect(Id("flex.reverse-rtl.", i).c_str(), children[i], LayoutRect(x, 0, widths[i] + 15, heights[i]));
          x += widths[i] + 15;
        }
        f.Arrange(LayoutRect(31, 17, total, 80));
        x = 0;
        for(unsigned i = 0; i < 8; ++i)
        {
          r.Rect(Id("flex.translated-parent.", i).c_str(), children[i], LayoutRect(x, 0, widths[i] + 15, heights[i]));
          x += widths[i] + 15;
        }
        Compute(v, 80, total);
        x = 0;
        for(unsigned i = 0; i < 8; ++i)
        {
          r.Rect(Id("flex.transposed.", i).c_str(), transposed[i], LayoutRect(0, x, heights[i], widths[i] + 15));
          x += widths[i] + 15;
        }
      }));
      out.push_back(Single(Id("R04.grid-seed.", seed).c_str(), "Seeded prefix tracks, RTL, translation and transpose", 192, [seed](Run& r)
      {
        uint32_t           state = seed;
        std::vector<float> rows, columns;
        float              height = 9, width = 10;
        for(unsigned i = 0; i < 4; ++i)
        {
          state = state * 1664525u + 1013904223u;
          rows.push_back(20 + state % 20);
          height += rows.back();
        }
        for(unsigned i = 0; i < 3; ++i)
        {
          state = state * 1664525u + 1013904223u;
          columns.push_back(15 + state % 20);
          width += columns.back();
        }
        auto g = GridLayout::New(), transpose = GridLayout::New();
        GridTracks(g, rows, columns);
        g.SetRowSpacing(3);
        g.SetColumnSpacing(5);
        GridTracks(transpose, columns, rows);
        transpose.SetRowSpacing(5);
        transpose.SetColumnSpacing(3);
        std::vector<View>       children, swapped;
        std::vector<LayoutRect> expected;
        float                   y = 0;
        for(unsigned row = 0; row < 4; ++row)
        {
          float x = 0;
          for(unsigned col = 0; col < 3; ++col)
          {
            children.push_back(Cell(g, row, col));
            swapped.push_back(Cell(transpose, col, row));
            expected.emplace_back(x, y, columns[col], rows[row]);
            x += columns[col] + 5;
          }
          y += rows[row] + 3;
        }
        Fixed(g, width, height);
        Fixed(transpose, height, width);
        Compute(g, width, height);
        for(unsigned i = 0; i < 12; ++i) r.Rect(Id("grid.ltr.", i).c_str(), children[i], expected[i]);
        g.SetLayoutDirection(LayoutDirection::RIGHT_TO_LEFT);
        Compute(g, width, height);
        for(unsigned i = 0; i < 12; ++i)
        {
          auto e = expected[i];
          e.x    = width - e.x - e.width;
          r.Rect(Id("grid.rtl.", i).c_str(), children[i], e);
        }
        g.Arrange(LayoutRect(31, 17, width, height));
        for(unsigned i = 0; i < 12; ++i)
        {
          auto e = expected[i];
          e.x    = width - e.x - e.width;
          r.Rect(Id("grid.translated-parent.", i).c_str(), children[i], e);
        }
        Compute(transpose, height, width);
        for(unsigned i = 0; i < 12; ++i) r.Rect(Id("grid.transposed.", i).c_str(), swapped[i], SwapAxes(expected[i]));
      }));
      out.push_back(Single(Id("R05.absolute-seed.", seed).c_str(), "Seeded bounds and proportional positions with rigid transformations", 192, [seed](Run& r)
      {
        uint32_t state = seed;
        auto     a = AbsoluteLayout::New(), transpose = AbsoluteLayout::New();
        Fixed(a, 200, 120);
        Fixed(transpose, 120, 200);
        std::vector<View>       children, swapped;
        std::vector<LayoutRect> expected;
        for(unsigned i = 0; i < 12; ++i)
        {
          state       = state * 1664525u + 1013904223u;
          float w     = 1 + state % 47;
          state       = state * 1664525u + 1013904223u;
          float h     = 1 + state % 31;
          state       = state * 1664525u + 1013904223u;
          float x     = (i % 2) ? (static_cast<int>(state % 7) - 1) * 0.25f : static_cast<int>(state % 50) - 10;
          state       = state * 1664525u + 1013904223u;
          float y     = static_cast<int>(state % 30) - 5;
          View  child = Leaf(w, h), other = Leaf(h, w);
          child.SetLayoutParams(AbsoluteLayoutParams::New().SetBounds(LayoutRect(x, y, w, h)).SetFlags((i % 2) ? AbsoluteLayoutFlags::X_PROPORTIONAL : AbsoluteLayoutFlags::NONE));
          other.SetLayoutParams(AbsoluteLayoutParams::New().SetBounds(LayoutRect(y, x, h, w)).SetFlags((i % 2) ? AbsoluteLayoutFlags::Y_PROPORTIONAL : AbsoluteLayoutFlags::NONE));
          a.Add(child);
          transpose.Add(other);
          children.push_back(child);
          swapped.push_back(other);
          expected.emplace_back((i % 2) ? (200 - w) * x : x, y, w, h);
        }
        Compute(a, 200, 120);
        for(unsigned i = 0; i < 12; ++i) r.Rect(Id("absolute.ltr.", i).c_str(), children[i], expected[i]);
        a.Arrange(LayoutRect(31, 17, 200, 120));
        for(unsigned i = 0; i < 12; ++i) r.Rect(Id("absolute.translated-parent.", i).c_str(), children[i], expected[i]);
        a.SetLayoutDirection(LayoutDirection::RIGHT_TO_LEFT);
        Compute(a, 200, 120);
        for(unsigned i = 0; i < 12; ++i)
        {
          auto e = expected[i];
          e.x    = 200 - e.x - e.width;
          r.Rect(Id("absolute.rtl.", i).c_str(), children[i], e);
        }
        Compute(transpose, 120, 200);
        for(unsigned i = 0; i < 12; ++i) r.Rect(Id("absolute.transposed.", i).c_str(), swapped[i], SwapAxes(expected[i]));
      }));
    }

    return out;
  }
};

REGISTER_MANUAL_TEST(TcLr58)
