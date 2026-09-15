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

class TcLr10 : public Case
{
public:
  TcLr10()
  : Case("LR10", "Stack allocation")
  {
  }
  std::vector<Scenario> BuildScenarios() override
  {
    std::vector<Scenario> out;
    for(bool vertical : {false, true})
    {
      out.push_back(Single(vertical ? "S01.vertical" : "S01.horizontal", "Stack order, gaps and weighted allocation", 12, [vertical](Run& r)
      {
        auto s = StackLayout::New(vertical ? StackOrientation::VERTICAL : StackOrientation::HORIZONTAL);
        s.SetSpacing(10);
        View a = Leaf(vertical ? 10 : 60, vertical ? 60 : 10), b = Leaf(20, 20), c = Leaf(30, 30);
        b.SetLayoutParams(StackLayoutParams::New().SetWeight(1));
        c.SetLayoutParams(StackLayoutParams::New().SetWeight(2));
        s.Add(a);
        s.Add(b);
        s.Add(c);
        Fixed(s, vertical ? 80 : 300, vertical ? 300 : 80);
        Compute(s, vertical ? 80 : 300, vertical ? 300 : 80);
        LayoutRect ea(0, 0, 60, 10), eb(70, 0, 220.0f / 3, 20), ec(80 + 220.0f / 3, 0, 440.0f / 3, 30);
        r.Rect("fixed", a, vertical ? SwapAxes(ea) : ea);
        r.Rect("weight1", b, vertical ? SwapAxes(eb) : eb);
        r.Rect("weight2", c, vertical ? SwapAxes(ec) : ec);
      }));
    }
    for(unsigned alignment = 0; alignment < 4; ++alignment)
      out.push_back(Single(Id("S04.cross.", alignment).c_str(), "Cross alignment with intrinsic child", 4, [alignment](Run& r)
      {
        auto s = StackLayout::New(StackOrientation::HORIZONTAL);
        Fixed(s, 100, 60);
        auto                  p        = Probed(r, 20, 10, false);
        const LayoutAlignment aligns[] = {LayoutAlignment::START, LayoutAlignment::CENTER, LayoutAlignment::END, LayoutAlignment::FILL};
        p->view.SetLayoutParams(StackLayoutParams::New().SetAlignment(aligns[alignment]));
        s.Add(p->view);
        Compute(s, 100, 60);
        r.Rect("aligned", p->view, LayoutRect(0, alignment == 1 ? 25 : alignment == 2 ? 50
                                                                                      : 0,
                                              20, alignment == 3 ? 60 : 10));
      }));
    out.push_back(Single("S05.margin-gap-overflow", "Margins and negative free space", 8, [](Run& r)
    {
      auto s = StackLayout::New(StackOrientation::HORIZONTAL);
      Fixed(s, 40, 50);
      s.SetSpacing(7);
      View a = Leaf(30, 10), b = Leaf(20, 15);
      a.SetMargin(Insets(2, 3, 4, 5));
      s.Add(a);
      s.Add(b);
      Compute(s, 40, 50);
      r.Rect("margin", a, LayoutRect(2, 4, 30, 10));
      r.Rect("overflow", b, LayoutRect(42, 0, 20, 15));
    }));
    return out;
  }
};

REGISTER_MANUAL_TEST(TcLr10)
