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

class TcLr23 : public Case
{
public:
  TcLr23()
  : Case("LR23", "Arrange replay")
  {
  }
  std::vector<Scenario> BuildScenarios() override
  {
    std::vector<Scenario> out;
    out.push_back(Single("K08.actor-repair", "Replay restores externally clobbered Actor geometry", 12, [](Run& r)
    {auto p=Probed(r);p->view.Measure(100,50);p->view.Arrange(LayoutRect(7,9,37,19));r.Rect("initial",p->view,LayoutRect(7,9,37,19));Actor raw=p->view;raw.SetProperty(Actor::Property::POSITION,Vector3(90,80,0));raw.SetProperty(Actor::Property::SIZE,Vector3(1,2,0));p->view.Arrange(LayoutRect(7,9,37,19));r.Rect("repaired",p->view,LayoutRect(7,9,37,19));r.Equal("producer-once",p->arranges,1);r.Near("pivot-x",raw.GetProperty<Vector3>(Actor::Property::PIVOT).x,0.5);r.Truth("placement-ignores-pivot",!raw.GetProperty<bool>(Actor::Property::POSITION_USES_PIVOT));r.Near("origin-x",raw.GetProperty<Vector3>(Actor::Property::PARENT_ORIGIN).x,0); }));
    out.push_back(Single("K09.subtree-replay", "Cached parent replays descendants without producers", 10, [](Run& r)
    {auto root=StackLayout::New(StackOrientation::HORIZONTAL);Fixed(root,200,60);auto a=Probed(r,20,10),b=Probed(r,30,15);root.Add(a->view);root.Add(b->view);Compute(root,200,60);uint32_t ac=a->arranges,bc=b->arranges;Actor raw=b->view;raw.SetProperty(Actor::Property::POSITION,Vector3(99,88,0));root.Arrange(LayoutRect(0,0,200,60));r.Rect("a",a->view,LayoutRect(0,0,20,10));r.Rect("b",b->view,LayoutRect(20,0,30,15));r.Equal("a-producer",a->arranges,ac);r.Equal("b-producer",b->arranges,bc); }));
    out.push_back(Single("K10.returned-bounds", "Valid producer result is authoritative on cache hit", 9, [](Run& r)
    {auto p=Probed(r);p->returned=[](const LayoutRect&){return LayoutRect(11,13,53,29);};p->view.Measure(100,50);r.Rect("returned",p->view.Arrange(LayoutRect(0,0,100,50)),LayoutRect(11,13,53,29));r.Rect("hit",p->view.Arrange(LayoutRect(0,0,100,50)),LayoutRect(11,13,53,29));r.Equal("producer",p->arranges,1); }));
    return out;
  }
};

REGISTER_MANUAL_TEST(TcLr23)
