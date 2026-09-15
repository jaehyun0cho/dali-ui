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

class TcLr09 : public Case
{
public:
  TcLr09()
  : Case("LR09", "Direction inheritance")
  {
  }
  std::vector<Scenario> BuildScenarios() override
  {
    std::vector<Scenario> out;
    out.push_back(Single("C15.LTR-RTL-LTR", "Parent mirror is applied once and reversible", 24, [](Run& r)
    {
      StackLayout p = StackLayout::New(StackOrientation::HORIZONTAL);
      Fixed(p, 200, 60);
      p.SetSpacing(7);
      View a = Leaf(20, 10), b = Leaf(30, 15);
      p.Add(a);
      p.Add(b);
      for(unsigned phase = 0; phase < 3; ++phase)
      {
        p.SetLayoutDirection(phase == 1 ? Dali::LayoutDirection::RIGHT_TO_LEFT : Dali::LayoutDirection::LEFT_TO_RIGHT);
        Compute(p, 200, 60);
        r.Rect(Id("a.", phase).c_str(), a, LayoutRect(phase == 1 ? 180 : 0, 0, 20, 10));
        r.Rect(Id("b.", phase).c_str(), b, LayoutRect(phase == 1 ? 143 : 27, 0, 30, 15));
      }
    }));
    out.push_back(Single("C15.explicit-child", "Explicit child direction preserves its own subtree rule", 8, [](Run& r)
    {
      View        root  = Leaf(300, 100);
      StackLayout child = StackLayout::New(StackOrientation::HORIZONTAL);
      Fixed(child, 100, 50);
      child.SetLayoutDirection(Dali::LayoutDirection::LEFT_TO_RIGHT);
      View leaf = Leaf(20, 10);
      child.Add(leaf);
      root.Add(child);
      root.SetLayoutDirection(Dali::LayoutDirection::RIGHT_TO_LEFT);
      Compute(root, 300, 100);
      r.Rect("child-mirrored-by-parent", child, LayoutRect(200, 0, 100, 50));
      r.Rect("leaf-explicit-ltr", leaf, LayoutRect(0, 0, 20, 10));
    }));
    return out;
  }
};

REGISTER_MANUAL_TEST(TcLr09)
