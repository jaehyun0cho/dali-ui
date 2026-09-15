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

class TcLr01 : public Case
{
public:
  TcLr01()
  : Case("LR01", "Core constraints")
  {
  }
  std::vector<Scenario> BuildScenarios() override
  {
    std::vector<Scenario> out;
    for(unsigned kind = 0; kind < 5; ++kind)
    {
      out.push_back(Single(Id("C01.empty.", kind).c_str(), "Empty layout and asymmetric padding", 4, [kind](Run& r)
      {
        View v = Container(kind);
        r.Size("empty", v.Measure(200, 100), MeasuredSize(0, 0));
        v.SetPadding(Insets(3, 5, 7, 11));
        r.Size("padded", v.Measure(200, 100), MeasuredSize(8, 18));
      }));
      for(float value : {0.0f, 1.0f, 37.5f, 120.0f})
        out.push_back(Single((Id("C02.fixed.", kind) + "." + std::to_string(value)).c_str(), "Fixed extent and authoritative slot", 6, [kind, value](Run& r)
        {
          View v = Container(kind, value, value);
          r.Size("measured", v.Measure(300, 200), MeasuredSize(value, value));
          r.Rect("arranged", v.Arrange(LayoutRect(7, 11, value, value)), LayoutRect(7, 11, value, value));
        }));
      for(float value : {39.0f, 40.0f, 41.0f, 99.0f, 100.0f, 101.0f})
        out.push_back(Single((Id("C07.clamp.", kind) + "." + std::to_string(value)).c_str(), "Independent min/max clamp", 2, [kind, value](Run& r)
        {
          View v = Container(kind, value, value);
          v.SetMinimumWidth(40);
          v.SetMaximumWidth(100);
          v.SetMinimumHeight(40);
          v.SetMaximumHeight(100);
          float expected = std::min(100.0f, std::max(40.0f, value));
          r.Size("clamp", v.Measure(300, 300), MeasuredSize(expected, expected));
        }));
      out.push_back(Single(Id("C08.equal-bounds.", kind).c_str(), "Equal minimum and maximum", 4, [kind](Run& r)
      {
        View v = Container(kind, 200, 0);
        v.SetMinimumWidth(60);
        v.SetMaximumWidth(60);
        v.SetMinimumHeight(30);
        v.SetMaximumHeight(30);
        r.Size("small-budget", v.Measure(1, 1), MeasuredSize(60, 30));
        r.Size("large-budget", v.Measure(400, 400), MeasuredSize(60, 30));
      }));
    }
    out.push_back(Single("C03.intrinsic", "Deterministic leaf versus constraint", 6, [](Run& r)
    {
      auto p = Probed(r);
      r.Size("free", p->view.Measure(100, 100), MeasuredSize(37, 19));
      r.Size("limited", p->view.Measure(20, 10), MeasuredSize(20, 10));
      r.Size("restored", p->view.Measure(100, 100), MeasuredSize(37, 19));
    }));
    return out;
  }
};

REGISTER_MANUAL_TEST(TcLr01)
