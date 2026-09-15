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

class TcLr13 : public Case
{
public:
  TcLr13()
  : Case("LR13", "Flex lines")
  {
  }
  std::vector<Scenario> BuildScenarios() override
  {
    std::vector<Scenario> out;
    for(bool reverse : {false, true})
      for(float width : {100.0f, 99.0f}) out.push_back(Single((std::string("F02.wrap.") + (reverse ? "reverse." : "normal.") + std::to_string(static_cast<int>(width))).c_str(), "Exact wrap boundary", 12, [reverse, width](Run& r)
      {
        auto f = FlexLayout::New();
        Fixed(f, width, 80);
        f.SetWrap(reverse ? FlexWrap::WRAP_REVERSE : FlexWrap::WRAP);
        f.SetAlignItems(FlexAlign::FLEX_START);
        f.SetAlignContent(FlexAlign::FLEX_START);
        View a = Leaf(40, 10), b = Leaf(60, 20), c = Leaf(30, 15);
        f.Add(a);
        f.Add(b);
        f.Add(c);
        Compute(f, width, 80);
        if(width == 100)
        {
          r.Rect("a", a, LayoutRect(0, reverse ? 15 : 0, 40, 10));
          r.Rect("b", b, LayoutRect(40, reverse ? 15 : 0, 60, 20));
          r.Rect("c", c, LayoutRect(0, reverse ? 0 : 20, 30, 15));
        }
        else
        {
          r.Rect("a", a, LayoutRect(0, reverse ? 20 : 0, 40, 10));
          r.Rect("b", b, LayoutRect(0, reverse ? 0 : 10, 60, 20));
          r.Rect("c", c, LayoutRect(60, reverse ? 0 : 10, 30, 15));
        }
      }));
    for(unsigned mode = 0; mode < 4; ++mode) out.push_back(Single(Id("F13.content.", mode).c_str(), "Wrapped line cross distribution", 8, [mode](Run& r)
    {
      auto f = FlexLayout::New();
      Fixed(f, 60, 100);
      f.SetWrap(FlexWrap::WRAP);
      f.SetAlignItems(FlexAlign::FLEX_START);
      const FlexAlign align[] = {FlexAlign::FLEX_START, FlexAlign::CENTER, FlexAlign::FLEX_END, FlexAlign::STRETCH};
      f.SetAlignContent(align[mode]);
      View a = Leaf(40, 10), b = Leaf(40, 20);
      f.Add(a);
      f.Add(b);
      Compute(f, 60, 100);
      float y0 = mode == 1 ? 35 : mode == 2 ? 70
                                            : 0,
            y1 = mode == 3 ? 45 : y0 + 10;
      r.Rect("line0", a, LayoutRect(0, y0, 40, 10));
      r.Rect("line1", b, LayoutRect(0, y1, 40, 20));
    }));
    return out;
  }
};

REGISTER_MANUAL_TEST(TcLr13)
