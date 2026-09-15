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

class TcLr14 : public Case
{
public:
  TcLr14()
  : Case("LR14", "Flex alignment")
  {
  }
  std::vector<Scenario> BuildScenarios() override
  {
    std::vector<Scenario> out;
    for(unsigned mode = 0; mode < 6; ++mode) out.push_back(Single(Id("F04.justify.", mode).c_str(), "Positive free space justify distribution", 8, [mode](Run& r)
    {
      auto f = FlexLayout::New();
      Fixed(f, 140, 60);
      f.SetAlignItems(FlexAlign::FLEX_START);
      f.SetJustifyContent(static_cast<FlexJustify>(mode));
      View a = Leaf(20, 10), b = Leaf(40, 10);
      f.Add(a);
      f.Add(b);
      Compute(f, 140, 60);
      const float ax[] = {0, 80, 40, 0, 20, 80.0f / 3}, bx[] = {20, 100, 60, 100, 80, 20 + 160.0f / 3};
      r.Rect("a", a, LayoutRect(ax[mode], 0, 20, 10));
      r.Rect("b", b, LayoutRect(bx[mode], 0, 40, 10));
    }));
    for(unsigned mode = 0; mode < 5; ++mode) out.push_back(Single(Id("F05.align-self.", mode).c_str(), "Cross alignment and align-self override", 8, [mode](Run& r)
    {
      auto f = FlexLayout::New();
      Fixed(f, 140, 60);
      f.SetAlignItems(FlexAlign::FLEX_END);
      View            a = Leaf(20, 10), b = Leaf(40, 10);
      const FlexAlign align[] = {FlexAlign::AUTO, FlexAlign::FLEX_START, FlexAlign::FLEX_END, FlexAlign::CENTER, FlexAlign::STRETCH};
      a.SetLayoutParams(FlexLayoutParams::New().SetAlignSelf(align[mode]));
      f.Add(a);
      f.Add(b);
      Compute(f, 140, 60);
      float y = mode == 0 || mode == 2 ? 50 : mode == 3 ? 25
                                                        : 0;
      r.Rect("self", a, LayoutRect(0, y, 20, mode == 4 ? 60 : 10));
      r.Rect("inherited", b, LayoutRect(20, 50, 40, 10));
    }));
    for(unsigned direction = 0; direction < 4; ++direction) out.push_back(Single(Id("F01.direction.", direction).c_str(), "Four directions", 8, [direction](Run& r)
    {
      auto f = FlexLayout::New();
      f.SetDirection(static_cast<FlexDirection>(direction));
      f.SetAlignItems(FlexAlign::FLEX_START);
      Fixed(f, 100, 100);
      bool vertical = direction >= 2, reverse = direction % 2;
      View a = Leaf(vertical ? 10 : 20, vertical ? 20 : 10), b = Leaf(vertical ? 10 : 30, vertical ? 30 : 10);
      f.Add(a);
      f.Add(b);
      Compute(f, 100, 100);
      LayoutRect ea(reverse ? 80 : 0, 0, 20, 10), eb(reverse ? 50 : 20, 0, 30, 10);
      r.Rect("a", a, vertical ? SwapAxes(ea) : ea);
      r.Rect("b", b, vertical ? SwapAxes(eb) : eb);
    }));

    for(unsigned mode : {3u, 4u, 5u}) out.push_back(Single(Id("F12.overflow.", mode).c_str(), "Negative free space does not create negative gaps", 8, [mode](Run& r)
    {auto f=FlexLayout::New();Fixed(f,40,50);f.SetAlignItems(FlexAlign::FLEX_START);f.SetJustifyContent(static_cast<FlexJustify>(mode));View a=Leaf(30,10),b=Leaf(30,10);a.SetLayoutParams(FlexLayoutParams::New().SetFlexShrink(0));b.SetLayoutParams(FlexLayoutParams::New().SetFlexShrink(0));f.Add(a);f.Add(b);Compute(f,40,50);float start=mode==3?0:-10;r.Rect("a",a,LayoutRect(start,0,30,10));r.Rect("b",b,LayoutRect(start+30,0,30,10)); }));

    return out;
  }
};

REGISTER_MANUAL_TEST(TcLr14)
