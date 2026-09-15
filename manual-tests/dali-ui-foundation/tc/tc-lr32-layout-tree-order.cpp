/* Copyright (c) 2026 Samsung Electronics Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy at http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 */
#include <dali-ui-foundation/integration-api/view-integ.h>
#include "layout-validation-fixtures.h"

using namespace LayoutValidation;
using namespace LayoutValidation::Fixtures;

class TcLr32 : public Case
{
public:
  TcLr32()
  : Case("LR32", "Tree and order")
  {
  }
  std::vector<Scenario> BuildScenarios() override
  {
    std::vector<Scenario> out;
    for(bool preserve : {false, true}) out.push_back(Single(preserve ? "ORDER.preserve" : "ORDER.update", "Visual order and layout order policy", 10, [preserve](Run& r)
    {auto s=StackLayout::New(StackOrientation::HORIZONTAL);Fixed(s,200,60);View a=Leaf(20,10),b=Leaf(30,10),c=Leaf(40,10);s.Add(a);s.Add(b);s.Add(c);Compute(s,200,60);a.RaiseAbove(c,preserve?LayoutOrderPolicy::PRESERVE:LayoutOrderPolicy::UPDATE);Compute(s,200,60);Identity(r,"first",s.GetChildViewAt(0),preserve?a:b);Identity(r,"last",s.GetChildViewAt(2),preserve?c:a);r.Rect("a",a,LayoutRect(preserve?0:70,0,20,10));r.Rect("b",b,LayoutRect(preserve?20:0,0,30,10)); }));
    out.push_back(Single("ORDER.non-view", "Non-View actor excluded from logical layout order", 7, [](Run& r)
    {auto s=StackLayout::New(StackOrientation::HORIZONTAL);View a=Leaf(20,10),b=Leaf(30,10);s.Add(a);Actor decoration=Actor::New();Dali::Ui::Integration::View::AddActorChild(s,decoration);s.Add(b);r.Equal("logical",s.GetChildViewCount(),2);r.Equal("actor",s.GetChildCount(),3);r.Truth("out-of-range",!s.GetChildViewAt(2));Compute(s,100,50);r.Rect("b",b,LayoutRect(20,0,30,10)); }));
    for(unsigned operation = 0; operation < 3; ++operation)
      out.push_back(Single(operation == 0 ? "ORDER.preserve-raise" : operation == 1 ? "ORDER.preserve-lower"
                                                                                    : "ORDER.preserve-lower-below",
                           "PRESERVE changes actual Actor order while retaining all logical slots", 20, [operation](Run& r)
      {
        auto root = StackLayout::New(StackOrientation::HORIZONTAL);
        Fixed(root, 200, 60);
        View a = Leaf(20, 10), b = Leaf(30, 10), c = Leaf(40, 10);
        root.Add(a);
        root.Add(b);
        root.Add(c);
        Compute(root, 200, 60);
        if(operation == 0)
          a.Raise(LayoutOrderPolicy::PRESERVE);
        else if(operation == 1)
          c.Lower(LayoutOrderPolicy::PRESERVE);
        else
          c.LowerBelow(a, LayoutOrderPolicy::PRESERVE);
        Compute(root, 200, 60);
        const View logical[]   = {a, b, c};
        const View visual[][3] = {{b, a, c}, {a, c, b}, {c, a, b}};
        r.Equal("logical.count", root.GetChildViewCount(), 3);
        r.Equal("actor.count", root.GetChildCount(), 3);
        for(unsigned i = 0; i < 3; ++i)
        {
          Identity(r, Id("logical.order", i).c_str(), root.GetChildViewAt(i), logical[i]);
          r.Truth(Id("actor.order", i).c_str(), root.GetChildAt(i) == visual[operation][i]);
        }
        r.Rect("preserved.a", a, LayoutRect(0, 0, 20, 10));
        r.Rect("preserved.b", b, LayoutRect(20, 0, 30, 10));
        r.Rect("preserved.c", c, LayoutRect(50, 0, 40, 10));
      }));
    return out;
  }
};

REGISTER_MANUAL_TEST(TcLr32)
