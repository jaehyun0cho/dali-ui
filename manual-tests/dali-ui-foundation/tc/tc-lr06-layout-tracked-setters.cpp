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

class TcLr06 : public Case
{
public:
  TcLr06()
  : Case("LR06", "Tracked setters")
  {
  }
  std::vector<Scenario> BuildScenarios() override
  {
    std::vector<Scenario>                        out;
    const std::vector<std::function<void(View)>> changes{[](View v)
    { v.SetRequestedWidth(80); }, [](View v)
    { v.SetPadding(Insets(2, 3, 4, 5)); }, [](View v)
    { v.SetMargin(Insets(1, 2, 3, 4)); }, [](View v)
    { v.SetMinimumWidth(5); }, [](View v)
    { v.SetLayoutParams(StackLayoutParams::New().SetWeight(2)); }, [](View v)
    { v.SetRequestedHeight(60); }, [](View v)
    { v.SetMinimumHeight(5); }, [](View v)
    { v.SetMaximumWidth(100); }, [](View v)
    { v.SetMaximumHeight(80); }, [](View v)
    { v.SetLayoutParams(FlexLayoutParams::New().SetFlexGrow(2)); }, [](View v)
    { v.SetLayoutParams(GridLayoutParams::New().SetRowSpan(2)); }, [](View v)
    { v.SetLayoutParams(AbsoluteLayoutParams::New().SetX(7)); }};
    for(unsigned i = 0; i < changes.size(); ++i) out.push_back(Single(Id("K05.tracked.", i).c_str(), "Changed setter invalidates; repeated identical value is inert", 3, [change = changes[i]](Run& r)
    {
      auto p = Probed(r);
      p->view.Measure(200, 100);
      uint32_t before = p->measures;
      change(p->view);
      p->view.Measure(200, 100);
      r.Equal("changed-producer", p->measures, before + 1);
      before = p->measures;
      change(p->view);
      p->view.Measure(200, 100);
      r.Equal("same-producer", p->measures, before);
      r.Near("measured-width", p->view.GetMeasuredSize().width, 37);
    }));
    out.push_back(Single("C19.hidden-participates", "Visibility does not collapse layout participation", 8, [](Run& r)
    {
      StackLayout p = StackLayout::New(StackOrientation::HORIZONTAL);
      Fixed(p, 200, 50);
      p.SetSpacing(7);
      View a = Leaf(20, 10), b = Leaf(30, 15);
      p.Add(a);
      p.Add(b);
      Compute(p, 200, 50);
      r.Rect("visible-b", b, LayoutRect(27, 0, 30, 15));
      a.SetVisible(false);
      Compute(p, 200, 50);
      r.Rect("hidden-b", b, LayoutRect(27, 0, 30, 15));
    }));
    return out;
  }
};

REGISTER_MANUAL_TEST(TcLr06)
