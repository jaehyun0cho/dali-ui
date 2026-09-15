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

class TcLr07 : public Case
{
public:
  TcLr07()
  : Case("LR07", "Numeric boundaries")
  {
  }
  std::vector<Scenario> BuildScenarios() override
  {
    std::vector<Scenario> out;
    out.push_back(Single("C20.measure-tolerance", "Normalized measure constraints use a strict 0.001 tolerance", 4, [](Run& r)
    {
      auto p = Probed(r);
      p->view.Measure(128, 100);
      auto n = p->measures;
      p->view.Measure(128.0005f, 100);
      r.Equal("below-epsilon-hit", p->measures, n);
      p->view.Measure(128.002f, 100);
      r.Equal("above-epsilon-miss", p->measures, n + 1);
      p->view.SetMinimumWidth(200);
      p->view.Measure(120, 100);
      n = p->measures;
      p->view.Measure(130, 100);
      r.Equal("normalized-hit", p->measures, n);
      r.Near("normalized-producer-input", p->lastWidth, 200, 0);
    }));
    out.push_back(Single("C20.pixel-setter", "Pixel setters preserve sub-epsilon stored values", 2, [](Run& r)
    {
      View v = Leaf(128, 19);
      v.SetRequestedWidth(128.0005f);
      r.Near("below-epsilon", v.GetRequestedWidth(), 128, 0);
      v.SetRequestedWidth(128.002f);
      r.Near("above-epsilon", v.GetRequestedWidth(), 128.002f, 0);
    }));
    out.push_back(Single("C20.exact-params", "Parameter identity must preserve a small mode change", 3, [](Run& r)
    {
      auto p = Probed(r);
      p->view.SetLayoutParams(StackLayoutParams::New().SetWeight(0));
      p->view.Measure(200, 100);
      auto n = p->measures;
      p->view.SetLayoutParams(StackLayoutParams::New().SetWeight(0.0005f));
      p->view.Measure(200, 100);
      r.Equal("mode-change-producer", p->measures, n + 1);
      StackLayoutParams params;
      r.Truth("params-present", p->view.TryGetLayoutParams(params));
      r.Near("exact-stored-weight", params.GetWeight(), 0.0005f, 0);
    }));
    out.push_back(Single("C20.exact-arrange", "Adjacent representable slot must not reuse old geometry", 3, [](Run& r)
    {
      auto p = Probed(r);
      p->view.Measure(200, 100);
      LayoutRect a(128, 0, 37, 19);
      p->view.Arrange(a);
      auto n = p->arranges;
      a.x    = std::nextafter(a.x, 200.0f);
      p->view.Arrange(a);
      r.Equal("changed-producer", p->arranges, n + 1);
      r.Near("exact-position", LayoutValidation::Bounds(p->view).x, a.x, 0);
      p->view.Arrange(a);
      r.Equal("unchanged-producer", p->arranges, n + 1);
    }));
    out.push_back(Single("C20.unmeasured-arrange", "Arrange cannot publish a cache before the first valid measure", 6, [](Run& r)
    {
      auto             p = Probed(r);
      const LayoutRect bounds(128, 0, 37, 19);
      p->view.Arrange(bounds);
      r.Equal("unmeasured.first-producer", p->arranges, 1);
      p->view.Arrange(bounds);
      r.Equal("unmeasured.repeat-producer", p->arranges, 2);
      r.Size("first-valid-measure", p->view.Measure(200, 100), MeasuredSize(37, 19));
      p->view.Arrange(bounds);
      r.Equal("measured.first-producer", p->arranges, 3);
      p->view.Arrange(bounds);
      r.Equal("measured.repeat-cache", p->arranges, 3);
    }));
    return out;
  }
};

REGISTER_MANUAL_TEST(TcLr07)
