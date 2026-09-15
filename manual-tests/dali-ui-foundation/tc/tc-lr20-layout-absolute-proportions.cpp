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

class TcLr20 : public Case
{
public:
  TcLr20()
  : Case("LR20", "Absolute proportions")
  {
  }
  std::vector<Scenario> BuildScenarios() override
  {
    std::vector<Scenario> out;
    for(unsigned bits = 0; bits < 16; ++bits) out.push_back(Single(Id("A04.flags.", bits).c_str(), "Independent proportional flags", 4, [bits](Run& r)
    {auto a=AbsoluteLayout::New();Fixed(a,200,100);View c=Leaf(10,10);c.SetLayoutParams(AbsoluteLayoutParams::New().SetBounds(LayoutRect(0.25f,0.5f,0.5f,0.25f)).SetFlags(static_cast<AbsoluteLayoutFlags>(bits)));a.Add(c);Compute(a,200,100);float w=bits&4?100:0.5f,h=bits&8?25:0.25f,x=bits&1?(200-w)*0.25f:0.25f,y=bits&2?(100-h)*0.5f:0.5f;r.Rect("flags",c,LayoutRect(x,y,w,h)); }));
    for(float position : {-0.25f, 0.0f, 0.5f, 1.0f, 1.25f}) out.push_back(Single((std::string("A08.position.") + std::to_string(position)).c_str(), "Proportional positions outside unit interval", 8, [position](Run& r)
    {auto a=AbsoluteLayout::New();Fixed(a,300,100);View c=Leaf(100,20);c.SetLayoutParams(AbsoluteLayoutParams::New().SetBounds(LayoutRect(position,0,100,20)).SetFlags(AbsoluteLayoutFlags::X_PROPORTIONAL));a.Add(c);Compute(a,300,100);r.Rect("ltr",c,LayoutRect(200*position,0,100,20));a.SetLayoutDirection(LayoutDirection::RIGHT_TO_LEFT);Compute(a,300,100);r.Rect("rtl",c,LayoutRect(200-200*position,0,100,20)); }));
    out.push_back(Single("A05.all-flags", "ALL includes all known proportional dimensions", 4, [](Run& r)
    {auto a=AbsoluteLayout::New();Fixed(a,200,100);View c=Leaf(10,10);c.SetLayoutParams(AbsoluteLayoutParams::New().SetBounds(LayoutRect(0.5f,0.5f,0.5f,0.5f)).SetFlags(AbsoluteLayoutFlags::ALL));a.Add(c);Compute(a,200,100);r.Rect("all",c,LayoutRect(50,25,100,50)); }));

    out.push_back(Single("A06.wrap-circularity", "Wrap keeps determinate extent and drops only circular contribution", 4, [](Run& r)
    {auto a=AbsoluteLayout::New();View c=Leaf(40,20);c.SetLayoutParams(AbsoluteLayoutParams::New().SetBounds(LayoutRect(0.5f,0,40,20)).SetFlags(AbsoluteLayoutFlags::X_PROPORTIONAL));a.Add(c);r.Size("position-only",a.Measure(200,100),MeasuredSize(40,20));c.SetLayoutParams(AbsoluteLayoutParams::New().SetBounds(LayoutRect(0,0,0.5f,20)).SetFlags(AbsoluteLayoutFlags::WIDTH_PROPORTIONAL));r.Size("size-circular",a.Measure(200,100),MeasuredSize(0,20)); }));

    return out;
  }
};

REGISTER_MANUAL_TEST(TcLr20)
