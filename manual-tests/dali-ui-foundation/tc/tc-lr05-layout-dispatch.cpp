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

class TcLr05 : public Case
{
public:
  TcLr05()
  : Case("LR05", "Layout dispatch")
  {
  }
  std::vector<Scenario> BuildScenarios() override
  {
    std::vector<Scenario> out;
    out.push_back(Single("C17.callback-manager-default", "Callback manager and default have distinct results", 6, [](Run& r)
    {
      auto p    = Probed(r, 7, 9, false);
      View root = p->view;
      root.Add(Leaf(20, 10));
      root.Add(Leaf(30, 15));
      root.SetMeasureCallback({});
      r.Size("default-before-attach", root.Measure(200, 100), MeasuredSize(30, 15));
      root.AttachLayoutManager(Dali::UniquePtr<LayoutManager>(new StackLayoutManager(StackOrientation::HORIZONTAL, 7)));
      root.SetMeasureCallback(MeasureCallback::New(p.get(), &Probe::Measure));
      r.Size("callback-priority", root.Measure(200, 100), MeasuredSize(7, 9));
      root.SetMeasureCallback({});
      r.Size("manager-after-remove", root.Measure(200, 100), MeasuredSize(57, 15));
    }));
    for(unsigned kind = 1; kind < 5; ++kind)
      out.push_back(Single(Id("C18.attached-manager.", kind).c_str(), "View plus manager matches public container", 8, [kind](Run& r)
      {
        View                           a = Container(kind, 200, 100), b = Leaf(200, 100);
        Dali::UniquePtr<LayoutManager> manager;
        if(kind == 1) manager = Dali::UniquePtr<LayoutManager>(new StackLayoutManager(StackOrientation::HORIZONTAL, 0));
        if(kind == 2) manager = Dali::UniquePtr<LayoutManager>(new FlexLayoutManager(FlexDirection::ROW, FlexWrap::NO_WRAP, FlexJustify::FLEX_START, FlexAlign::STRETCH, FlexAlign::STRETCH));
        if(kind == 3) manager = Dali::UniquePtr<LayoutManager>(new GridLayoutManager({}, {}, 0, 0));
        if(kind == 4) manager = Dali::UniquePtr<LayoutManager>(new AbsoluteLayoutManager());
        b.AttachLayoutManager(std::move(manager));
        View ca = Leaf(20, 10), cb = Leaf(20, 10);
        a.Add(ca);
        b.Add(cb);
        Compute(a, 200, 100);
        Compute(b, 200, 100);
        LayoutRect expected(0, 0, 20, 10);
        if(kind == 2) expected.height = 100;
        if(kind == 3)
        {
          expected.width  = 200;
          expected.height = 100;
        }
        r.Rect("container", ca, expected);
        r.Rect("attached", cb, expected);
      }));
    return out;
  }
};

REGISTER_MANUAL_TEST(TcLr05)
