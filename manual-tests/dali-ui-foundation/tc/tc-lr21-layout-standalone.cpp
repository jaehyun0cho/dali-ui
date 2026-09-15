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

class TcLr21 : public Case
{
public:
  TcLr21()
  : Case("LR21", "Standalone layout")
  {
  }
  std::vector<Scenario> BuildScenarios() override
  {
    std::vector<Scenario> out;
    for(unsigned kind = 0; kind < 5; ++kind) out.push_back(Single(Id("S10.standalone.", kind).c_str(), "Standalone excluded from accumulation and positioned from requests", 7, [kind](Run& r)
    {View root=Container(kind);View ordinary=Leaf(20,10),standalone=Leaf(100,80);standalone.SetLayoutMode(LayoutMode::STANDALONE);standalone.SetRequestedX(7);standalone.SetRequestedY(9);root.Add(ordinary);root.Add(standalone);r.Size("parent-measured",root.Measure(200,100),MeasuredSize(20,10));standalone.Measure(100,80);standalone.Arrange(LayoutRect(77,99,100,80));root.Arrange(LayoutRect(0,0,200,100));r.Rect("independent",standalone,LayoutRect(7,9,100,80));r.Equal("logical-count",root.GetChildViewCount(),2); }));
    out.push_back(Single("G14.mode-change", "Changing mode invalidates parent intrinsic extent", 4, [](Run& r)
    {auto s=StackLayout::New(StackOrientation::HORIZONTAL);s.SetSpacing(7);View a=Leaf(20,10),b=Leaf(30,15);s.Add(a);s.Add(b);r.Size("ordinary",s.Measure(200,100),MeasuredSize(57,15));b.SetLayoutMode(LayoutMode::STANDALONE);r.Size("excluded",s.Measure(200,100),MeasuredSize(20,10)); }));
    for(unsigned kind = 0; kind < 5; ++kind)
      out.push_back(Single(Id("S10.match-margin-clamp.", kind).c_str(), "Standalone MATCH_PARENT ignores parent padding and applies its own margin and min/max", 8, [kind](Run& r)
      {
        View root = Container(kind, 200, 100);
        root.SetPadding(Insets(20, 30, 40, 10));
        View child = Leaf(MATCH_PARENT, MATCH_PARENT);
        child.SetLayoutMode(LayoutMode::STANDALONE);
        child.SetRequestedX(5);
        child.SetRequestedY(9);
        child.SetMargin(Insets(7, 11, 13, 17));
        root.Add(Leaf(20, 10));
        root.Add(child);
        Compute(root, 200, 100);
        r.Rect("full-parent-minus-own-margin", child, LayoutRect(12, 22, 182, 70));
        child.SetMinimumWidth(190);
        child.SetMaximumWidth(195);
        child.SetMinimumHeight(20);
        child.SetMaximumHeight(60);
        Compute(root, 200, 100);
        r.Rect("own-minmax-after-match", child, LayoutRect(12, 22, 190, 60));
      }));
    return out;
  }
};

REGISTER_MANUAL_TEST(TcLr21)
