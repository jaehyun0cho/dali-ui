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

class TcLr16 : public Case
{
public:
  TcLr16()
  : Case("LR16", "Grid tracks")
  {
  }
  std::vector<Scenario> BuildScenarios() override
  {
    std::vector<Scenario> out;
    out.push_back(Single("G01.absolute", "Absolute tracks and spacing", 12, [](Run& r)
    {auto g=GridLayout::New();GridTracks(g,{20,30},{40,50});g.SetRowSpacing(5);g.SetColumnSpacing(7);View a=Cell(g,0,0),b=Cell(g,0,1),c=Cell(g,1,1);Compute(g,200,100);r.Rect("a",a,LayoutRect(0,0,40,20));r.Rect("b",b,LayoutRect(47,0,50,20));r.Rect("c",c,LayoutRect(47,25,50,30)); }));
    out.push_back(Single("G02.star", "Fixed plus star conservation", 12, [](Run& r)
    {auto g=GridLayout::New();Fixed(g,330,50);g.AddColumnDefinition(GridLength::Absolute(50));g.AddColumnDefinition(GridLength::Star(1));g.AddColumnDefinition(GridLength::Star(2));g.SetColumnSpacing(10);View a=Cell(g,0,0),b=Cell(g,0,1),c=Cell(g,0,2);Compute(g,330,50);r.Rect("a",a,LayoutRect(0,0,50,50));r.Rect("b",b,LayoutRect(60,0,260.0f/3,50));r.Rect("c",c,LayoutRect(70+260.0f/3,0,520.0f/3,50)); }));
    out.push_back(Single("G03.auto", "Auto floors and remaining star", 8, [](Run& r)
    {auto g=GridLayout::New();Fixed(g,200,50);g.AddColumnDefinition(GridLength::Auto());g.AddColumnDefinition(GridLength::Star());g.SetColumnSpacing(10);View a=Cell(g,0,0),b=Cell(g,0,1);Fixed(a,40,10);Compute(g,200,50);r.Rect("auto",a,LayoutRect(0,0,40,50));r.Rect("star",b,LayoutRect(50,0,150,50)); }));
    out.push_back(Single("G08.zero-star-overflow", "Zero star budget and fixed overflow", 8, [](Run& r)
    {auto g=GridLayout::New();Fixed(g,40,50);g.AddColumnDefinition(GridLength::Absolute(60));g.AddColumnDefinition(GridLength::Star(0));g.SetColumnSpacing(7);View a=Cell(g,0,0),b=Cell(g,0,1);Compute(g,40,50);r.Rect("absolute",a,LayoutRect(0,0,60,50));r.Rect("zero",b,LayoutRect(67,0,0,50)); }));
    out.push_back(Single("G04.implicit", "Implicit single track and definitions replacement", 8, [](Run& r)
    {auto g=GridLayout::New();Fixed(g,100,60);View a=Cell(g,0,0);Compute(g,100,60);r.Rect("implicit",a,LayoutRect(0,0,100,60));GridTracks(g,{20},{30});Compute(g,100,60);r.Rect("explicit",a,LayoutRect(0,0,30,20)); }));
    return out;
  }
};

REGISTER_MANUAL_TEST(TcLr16)
