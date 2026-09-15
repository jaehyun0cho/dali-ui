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

class TcLr18 : public Case
{
public:
  TcLr18()
  : Case("LR18", "Grid boundaries")
  {
  }
  std::vector<Scenario> BuildScenarios() override
  {
    std::vector<Scenario> out;
    for(uint32_t index : {0u, 1u, UINT32_MAX}) out.push_back(Single(Id("G12.index.", index).c_str(), "First, last and clamped indexes", 4, [index](Run& r)
    {auto g=GridLayout::New();GridTracks(g,{20,30},{30,50});g.SetRowSpacing(5);g.SetColumnSpacing(7);View a=Cell(g,index,index);Compute(g,100,70);r.Rect("cell",a,index==0?LayoutRect(0,0,30,20):LayoutRect(37,25,50,30)); }));
    for(uint32_t span : {0u, 1u, 2u, UINT32_MAX}) out.push_back(Single(Id("G12.span.", span).c_str(), "Minimum and clamped span", 6, [span](Run& r)
    {auto g=GridLayout::New();GridTracks(g,{20,30},{30,50});g.SetRowSpacing(5);g.SetColumnSpacing(7);View a=Cell(g,0,0,span,span);GridLayoutParams p;r.Truth("params",a.TryGetLayoutParams(p));r.Equal("normalized-span",p.GetRowSpan(),span==0?1:span);Compute(g,100,70);r.Rect("span",a,span<=1?LayoutRect(0,0,30,20):LayoutRect(0,0,87,55)); }));
    out.push_back(Single("G13.definition-clear", "Clear and restore tracks", 8, [](Run& r)
    {auto g=GridLayout::New();Fixed(g,100,60);GridTracks(g,{20},{30});View a=Cell(g,UINT32_MAX,UINT32_MAX);Compute(g,100,60);r.Rect("defined",a,LayoutRect(0,0,30,20));g.ClearRowDefinitions();g.ClearColumnDefinitions();Compute(g,100,60);r.Rect("cleared",a,LayoutRect(0,0,100,60)); }));
    return out;
  }
};

REGISTER_MANUAL_TEST(TcLr18)
