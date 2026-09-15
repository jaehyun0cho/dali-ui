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

class TcLr24 : public Case
{
public:
  TcLr24()
  : Case("LR24", "Arrange policy")
  {
  }
  std::vector<Scenario> BuildScenarios() override
  {
    std::vector<Scenario> out;
    for(bool always : {false, true}) out.push_back(Single(always ? "K15.always" : "K15.if-changed", "Explicit callback policy", 5, [always](Run& r)
    {auto p=Probed(r);p->view.SetArrangeCallback(ArrangeCallback::New(p.get(),&Probe::Arrange),always?ArrangePolicy::ALWAYS:ArrangePolicy::IF_CHANGED);p->view.Measure(100,50);for(int i=0;i<3;++i)p->view.Arrange(LayoutRect(0,0,37,19));r.Equal("producer",p->arranges,always?3:1);r.Rect("bounds",p->view,LayoutRect(0,0,37,19)); }));
    out.push_back(Single("K16.always-descendant", "ALWAYS descendant defeats ancestor shortcut", 10, [](Run& r)
    {auto s=StackLayout::New(StackOrientation::HORIZONTAL);Fixed(s,100,40);auto a=Probed(r,20,10),b=Probed(r,30,10);a->view.SetArrangeCallback(ArrangeCallback::New(a.get(),&Probe::Arrange),ArrangePolicy::ALWAYS);s.Add(a->view);s.Add(b->view);Compute(s,100,40);uint32_t ac=a->arranges,bc=b->arranges;s.Arrange(LayoutRect(0,0,100,40));r.Equal("always-runs",a->arranges,ac+1);r.Equal("clean-sibling-hit",b->arranges,bc);r.Rect("a",a->view,LayoutRect(0,0,20,10));r.Rect("b",b->view,LayoutRect(20,0,30,10)); }));
    out.push_back(Single("K16.policy-change", "Replacing callback changes policy and invalidates", 2, [](Run& r)
    {auto p=Probed(r);p->view.Measure(100,50);p->view.Arrange(LayoutRect(0,0,37,19));p->view.SetArrangeCallback(ArrangeCallback::New(p.get(),&Probe::Arrange),ArrangePolicy::ALWAYS);p->view.Arrange(LayoutRect(0,0,37,19));p->view.Arrange(LayoutRect(0,0,37,19));r.Equal("always-total",p->arranges,3);p->view.SetArrangeCallback(ArrangeCallback::New(p.get(),&Probe::Arrange));p->view.Arrange(LayoutRect(0,0,37,19));p->view.Arrange(LayoutRect(0,0,37,19));r.Equal("if-changed-total",p->arranges,4); }));
    return out;
  }
};

REGISTER_MANUAL_TEST(TcLr24)
