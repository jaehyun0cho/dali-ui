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

class TcLr19 : public Case
{
public:
  TcLr19()
  : Case("LR19", "Absolute bounds")
  {
  }
  std::vector<Scenario> BuildScenarios() override
  {
    std::vector<Scenario> out;
    out.push_back(Single("A01.explicit-local", "Explicit bounds and local origin", 8, [](Run& r)
    {auto a=AbsoluteLayout::New();Fixed(a,200,100);View c=Leaf(10,10);c.SetLayoutParams(AbsoluteLayoutParams::New().SetBounds(LayoutRect(11,13,37,19)));a.Add(c);a.Measure(200,100);a.Arrange(LayoutRect(70,90,200,100));r.Rect("child",c,LayoutRect(11,13,37,19));a.Arrange(LayoutRect(100,120,200,100));r.Rect("moved-parent",c,LayoutRect(11,13,37,19)); }));
    for(float extent : {0.0f, 1.0f, 37.0f, 250.0f}) out.push_back(Single((std::string("A02.extent.") + std::to_string(static_cast<int>(extent))).c_str(), "Explicit extent includes zero and overflow", 4, [extent](Run& r)
    {auto a=AbsoluteLayout::New();Fixed(a,100,60);View c=Leaf(10,10);c.SetLayoutParams(AbsoluteLayoutParams::New().SetBounds(LayoutRect(-20,7,extent,19)));a.Add(c);Compute(a,100,60);r.Rect("explicit",c,LayoutRect(-20,7,extent,19)); }));
    out.push_back(Single("A03.intrinsic-match", "Negative extent delegates to requested size", 8, [](Run& r)
    {auto a=AbsoluteLayout::New();Fixed(a,100,60);View c=Leaf(37,19);c.SetLayoutParams(AbsoluteLayoutParams::New().SetBounds(LayoutRect(5,7,-1,-1)));a.Add(c);Compute(a,100,60);r.Rect("intrinsic",c,LayoutRect(5,7,37,19));Fixed(c,MATCH_PARENT,MATCH_PARENT);Compute(a,100,60);r.Rect("match",c,LayoutRect(5,7,100,60)); }));
    out.push_back(Single("A07.wrap-negative-position", "Wrap uses positive far edge", 2, [](Run& r)
    {auto a=AbsoluteLayout::New();View c=Leaf(100,20);c.SetLayoutParams(AbsoluteLayoutParams::New().SetBounds(LayoutRect(-20,-5,100,20)));a.Add(c);r.Size("wrap",a.Measure(300,100),MeasuredSize(80,15)); }));
    return out;
  }
};

REGISTER_MANUAL_TEST(TcLr19)
