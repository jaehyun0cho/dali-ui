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

class TcLr11 : public Case
{
public:
  TcLr11()
  : Case("LR11", "Stack wrap and clamp")
  {
  }
  std::vector<Scenario> BuildScenarios() override
  {
    std::vector<Scenario> out;
    for(bool maximum : {false, true}) out.push_back(Single(maximum ? "S08.maximum" : "S08.minimum", "Measure clamp is distinct from allocation", 10, [maximum](Run& r)
    {
      auto s = StackLayout::New(StackOrientation::HORIZONTAL);
      Fixed(s, 200, 60);
      View a = Leaf(maximum ? 160 : 20, 10), b = Leaf(20, 10);
      a.SetLayoutParams(StackLayoutParams::New().SetWeight(1));
      b.SetLayoutParams(StackLayoutParams::New().SetWeight(1));
      if(maximum)
        a.SetMaximumWidth(60);
      else
        a.SetMinimumWidth(120);
      s.Add(a);
      s.Add(b);
      Compute(s, 200, 60);
      r.Size("measured", a.GetMeasuredSize(), MeasuredSize(maximum ? 60 : 120, 10));
      r.Rect("allocated-a", a, LayoutRect(0, 0, 100, 10));
      r.Rect("allocated-b", b, LayoutRect(100, 0, 100, 10));
    }));
    out.push_back(Single("S06.wrap-weight", "Wrap intrinsic fallback and minimum allocation", 4, [](Run& r)
    {
      auto s = StackLayout::New(StackOrientation::HORIZONTAL);
      s.SetSpacing(10);
      View a = Leaf(20, 10), b = Leaf(30, 15);
      b.SetLayoutParams(StackLayoutParams::New().SetWeight(1));
      s.Add(a);
      s.Add(b);
      r.Size("wrap", s.Measure(300, 100), MeasuredSize(60, 15));
      s.SetMinimumWidth(100);
      r.Size("minimum", s.Measure(300, 100), MeasuredSize(100, 15));
    }));
    out.push_back(Single("S07.match-overflow", "Unweighted MATCH_PARENT and weighted MATCH_PARENT", 8, [](Run& r)
    {
      auto s = StackLayout::New(StackOrientation::HORIZONTAL);
      Fixed(s, 100, 40);
      View a = Leaf(20, 10), b = Leaf(MATCH_PARENT, 10);
      s.Add(a);
      s.Add(b);
      Compute(s, 100, 40);
      r.Rect("unweighted", b, LayoutRect(20, 0, 100, 10));
      b.SetLayoutParams(StackLayoutParams::New().SetWeight(1));
      Compute(s, 100, 40);
      r.Rect("weighted", b, LayoutRect(20, 0, 80, 10));
    }));
    return out;
  }
};

REGISTER_MANUAL_TEST(TcLr11)
