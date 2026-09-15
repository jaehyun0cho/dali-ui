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

class TcLr22 : public Case
{
public:
  TcLr22()
  : Case("LR22", "Measure cache")
  {
  }
  std::vector<Scenario> BuildScenarios() override
  {
    std::vector<Scenario> out;
    out.push_back(Single("K01.same-key", "Cold miss and identical hit", 6, [](Run& r)
    {auto p=Probed(r);r.Size("cold",p->view.Measure(100,50),MeasuredSize(37,19));r.Size("hit",p->view.Measure(100,50),MeasuredSize(37,19));r.Equal("producer-once",p->measures,1);p->view.Measure(100,51);r.Equal("height-key",p->measures,2); }));
    out.push_back(Single("K02.normalized-key", "Different raw constraints normalize to one key", 5, [](Run& r)
    {auto p=Probed(r);p->view.SetMinimumWidth(120);p->view.SetMaximumHeight(30);p->view.Measure(40,100);p->view.Measure(50,200);r.Equal("producer-once",p->measures,1);r.Near("normalized-width",p->lastWidth,120);r.Near("normalized-height",p->lastHeight,30);r.Size("clamped-result",p->view.GetMeasuredSize(),MeasuredSize(120,19)); }));
    out.push_back(Single("K03.explicit-invalidation", "External content requires invalidation", 7, [](Run& r)
    {auto p=Probed(r);p->view.Measure(100,50);p->width=53;r.Size("cached",p->view.Measure(100,50),MeasuredSize(37,19));r.Equal("unchanged-producer",p->measures,1);p->view.InvalidateMeasure();r.Size("invalidated",p->view.Measure(100,50),MeasuredSize(53,19));r.Equal("fresh-producer",p->measures,2);p->view.Measure(100,50);r.Equal("fresh-cache",p->measures,2); }));
    out.push_back(Single("K04.sibling-isolation", "One dirty child preserves clean sibling cache", 4, [](Run& r)
    {auto s=StackLayout::New(StackOrientation::HORIZONTAL);auto a=Probed(r,20,10,false),b=Probed(r,30,10,false);s.Add(a->view);s.Add(b->view);s.Measure(200,100);uint32_t ac=a->measures,bc=b->measures;a->width=40;a->view.InvalidateMeasure();r.Size("root",s.Measure(200,100),MeasuredSize(70,10));r.Equal("dirty-producer",a->measures,ac+1);r.Equal("clean-producer",b->measures,bc); }));
    return out;
  }
};

REGISTER_MANUAL_TEST(TcLr22)
