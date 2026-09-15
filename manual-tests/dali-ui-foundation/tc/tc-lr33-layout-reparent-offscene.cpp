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

class TcLr33 : public Case
{
public:
  TcLr33()
  : Case("LR33", "Reparent and offscene")
  {
  }
  std::vector<Scenario> BuildScenarios() override
  {
    std::vector<Scenario> out;
    out.push_back(Single("L01.reparent", "Reparent invalidates old and new intrinsic roots", 12, [](Run& r)
    {auto left=StackLayout::New(StackOrientation::HORIZONTAL),right=StackLayout::New(StackOrientation::HORIZONTAL);left.SetSpacing(7);right.SetSpacing(5);View a=Leaf(20,10),b=Leaf(30,15),c=Leaf(40,20);left.Add(a);left.Add(b);right.Add(c);left.Measure(200,100);right.Measure(200,100);right.Add(b);r.Size("old-root",left.Measure(200,100),MeasuredSize(20,10));r.Size("new-root",right.Measure(200,100),MeasuredSize(75,20));r.Equal("old-count",left.GetChildViewCount(),1);r.Equal("new-count",right.GetChildViewCount(),2);Compute(right,200,100);r.Rect("moved",b,LayoutRect(45,0,30,15));Identity(r,"parent",View::DownCast(b.GetParent()),right);Identity(r,"left-child",left.GetChildViewAt(0),a); }));
    out.push_back(Single("L02.detached-measure", "Detached tree retains deterministic layout", 8, [](Run& r)
    {View root=View::New(),child=Leaf(37,19);child.SetRequestedX(7);child.SetRequestedY(9);root.Add(child);r.Size("attached-to-root",root.Measure(100,60),MeasuredSize(44,28));root.Remove(child);r.Size("empty",root.Measure(100,60),MeasuredSize(0,0));r.Size("detached-child",child.Measure(100,60),MeasuredSize(37,19));root.Add(child);r.Size("reattached",root.Measure(100,60),MeasuredSize(44,28)); }));
    return out;
  }
};

REGISTER_MANUAL_TEST(TcLr33)
