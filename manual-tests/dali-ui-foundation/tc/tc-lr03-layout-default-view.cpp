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

class TcLr03 : public Case
{
public:
  TcLr03()
  : Case("LR03", "Default View")
  {
  }
  std::vector<Scenario> BuildScenarios() override
  {
    std::vector<Scenario> out;
    out.push_back(Single("C10.C12.local-coordinate", "Parent offset is not duplicated into child local bounds", 12, [](Run& r)
    {
      View p = Leaf(200, 150), c = Leaf(37, 19);
      p.SetPadding(Insets(3, 5, 7, 11));
      c.SetMargin(Insets(2, 4, 6, 8));
      c.SetRequestedX(11);
      c.SetRequestedY(13);
      p.Add(c);
      p.Measure(200, 150);
      p.Arrange(LayoutRect(70, 90, 200, 150));
      r.Rect("parent", p, LayoutRect(70, 90, 200, 150));
      r.Rect("child", c, LayoutRect(16, 26, 37, 19));
      p.Arrange(LayoutRect(100, 120, 200, 150));
      r.Rect("child-after-parent-move", c, LayoutRect(16, 26, 37, 19));
    }));
    out.push_back(Single("C04.wrap-bounds", "Default View uses child bounding extent", 4, [](Run& r)
    {
      View p = View::New(), a = Leaf(20, 10), b = Leaf(30, 15);
      a.SetRequestedX(5);
      a.SetRequestedY(7);
      b.SetRequestedX(40);
      b.SetRequestedY(20);
      p.Add(a);
      p.Add(b);
      r.Size("wrap", p.Measure(200, 100), MeasuredSize(70, 35));
      b.SetRequestedWidth(40);
      r.Size("changed", p.Measure(200, 100), MeasuredSize(80, 35));
    }));
    out.push_back(Single("C05.C11.match-budget", "Match parent fills padding and margin adjusted slot", 6, [](Run& r)
    {
      View p = Leaf(100, 60), c = Leaf(MATCH_PARENT, MATCH_PARENT);
      p.SetPadding(Insets(3, 5, 7, 11));
      c.SetMargin(Insets(2, 4, 6, 8));
      p.Add(c);
      Compute(p, 100, 60);
      r.Size("minimum-contribution", c.GetMeasuredSize(), MeasuredSize(0, 0));
      r.Rect("match-slot", c, LayoutRect(5, 13, 86, 28));
    }));
    out.push_back(Single("C06.wrap-match", "Match child contributes minimum to wrap parent", 4, [](Run& r)
    {
      View p = View::New(), c = Leaf(MATCH_PARENT, MATCH_PARENT);
      c.SetMinimumWidth(30);
      c.SetMinimumHeight(20);
      p.Add(c);
      r.Size("wrap-minimum", p.Measure(200, 100), MeasuredSize(30, 20));
      c.SetMinimumWidth(10);
      r.Size("reduced-minimum", p.Measure(200, 100), MeasuredSize(10, 20));
    }));

    out.push_back(Single("C11.exhausted-padding", "Padding and margin exhaust the content budget", 6, [](Run& r)
    {View parent=Leaf(10,10),child=Leaf(MATCH_PARENT,MATCH_PARENT);parent.SetPadding(Insets(8,8,8,8));child.SetMargin(Insets(3,3,3,3));parent.Add(child);Compute(parent,10,10);r.Size("zero-measure",child.GetMeasuredSize(),MeasuredSize(0,0));r.Rect("zero-content",child,LayoutRect(11,11,0,0)); }));

    return out;
  }
};

REGISTER_MANUAL_TEST(TcLr03)
