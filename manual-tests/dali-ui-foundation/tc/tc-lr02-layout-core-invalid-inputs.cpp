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

class TcLr02 : public Case
{
public:
  TcLr02()
  : Case("LR02", "Core invalid inputs")
  {
  }
  std::vector<Scenario> BuildScenarios() override
  {
    std::vector<Scenario>      out;
    const std::array<float, 4> values{{-3.0f, std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity()}};
    for(unsigned i = 0; i < values.size(); ++i)
      out.push_back(Single(Id("C09.reject.", i).c_str(), "Rejected size properties preserve prior state", 8, [v = values[i]](Run& r)
      {
        View a = Leaf(37, 19);
        a.SetMinimumWidth(3);
        a.SetMaximumWidth(100);
        a.SetMinimumHeight(2);
        a.SetMaximumHeight(80);
        a.SetRequestedWidth(v);
        a.SetRequestedHeight(v);
        a.SetMinimumWidth(v);
        a.SetMaximumWidth(v);
        a.SetMinimumHeight(v);
        a.SetMaximumHeight(v);
        r.Near("requested-width", a.GetRequestedWidth(), 37, 0);
        r.Near("requested-height", a.GetRequestedHeight(), 19, 0);
        r.Near("minimum-width", a.GetMinimumWidth(), 3, 0);
        r.Near("maximum-width", a.GetMaximumWidth(), 100, 0);
        r.Near("minimum-height", a.GetMinimumHeight(), 2, 0);
        r.Near("maximum-height", a.GetMaximumHeight(), 80, 0);
        r.Size("preserved-measure", a.Measure(200, 100), MeasuredSize(37, 19));
      }));
    for(float requested : {20.0f, 70.0f, 150.0f})
      out.push_back(Single(("C09.max-wins." + std::to_string(requested)).c_str(), "Conflicting bounds have a documented maximum winner", 2, [requested](Run& r)
      {
        View a = Leaf(requested, requested);
        a.SetMinimumWidth(100);
        a.SetMaximumWidth(40);
        a.SetMinimumHeight(90);
        a.SetMaximumHeight(30);
        r.Size("maximum-wins", a.Measure(300, 300), MeasuredSize(40, 30));
      }));
    for(float sentinel : {WRAP_CONTENT, MATCH_PARENT})
      out.push_back(Single(("C09.sentinel." + std::to_string(sentinel)).c_str(), "Near sentinel is canonicalized", 2, [sentinel](Run& r)
      {
        View a = Leaf(37, 19);
        a.SetRequestedWidth(sentinel + 0.0005f);
        a.SetRequestedHeight(sentinel - 0.0005f);
        r.Near("canonical-width", a.GetRequestedWidth(), sentinel, 0);
        r.Near("canonical-height", a.GetRequestedHeight(), sentinel, 0);
      }));
    out.push_back(Single("R03.negative-flex-factors", "Negative flex factors normalize to zero", 2, [](Run& r)
    {
      auto p = FlexLayoutParams::New().SetFlexGrow(-1).SetFlexShrink(-2);
      r.Near("grow", p.GetFlexGrow(), 0, 0);
      r.Near("shrink", p.GetFlexShrink(), 0, 0);
    }));

    for(unsigned invalid = 0; invalid < 3; ++invalid) out.push_back(Single(Id("R03.returned-rect.", invalid).c_str(), "Reject the entire invalid custom result", 9, [invalid](Run& r)
    {auto p=Probed(r);p->view.Arrange(LayoutRect(1,2,37,19));p->returned=[invalid](const LayoutRect&){return invalid==0?LayoutRect(0,0,-1,19):invalid==1?LayoutRect(std::numeric_limits<float>::quiet_NaN(),0,37,19):LayoutRect(0,0,37,std::numeric_limits<float>::infinity());};bool caught=false;LayoutRect result;try{result=p->view.Arrange(LayoutRect(7,9,53,29));}catch(const DaliException&){caught=true;}r.Truth("invalid-result-not-published",caught||(result.x==7&&result.y==9&&result.width==53&&result.height==29));r.Rect("safe-actor",p->view,LayoutRect(7,9,53,29));p->returned={};p->view.InvalidateArrange();r.Rect("recovered",p->view.Arrange(LayoutRect(11,13,61,31)),LayoutRect(11,13,61,31)); }));

    return out;
  }
};

REGISTER_MANUAL_TEST(TcLr02)
