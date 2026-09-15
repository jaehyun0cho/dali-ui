/* Copyright (c) 2026 Samsung Electronics Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy at http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 */
#include <dali-ui-foundation/public-api/views/view-impl.h>
#include "layout-validation-fixtures.h"

using namespace LayoutValidation;
using namespace LayoutValidation::Fixtures;

class TcLr30 : public Case
{
public:
  TcLr30()
  : Case("LR30", "Reentry and exceptions")
  {
  }
  std::vector<Scenario> BuildScenarios() override
  {
    std::vector<Scenario> out;
    out.push_back(Single("K18.measure-throw", "Throw rolls back publication and permits retry", 5, [](Run& r)
    {auto p=Probed(r);p->view.Measure(100,50);p->view.InvalidateMeasure();p->onMeasure=[](){throw std::runtime_error("LR30 expected producer exception");};bool caught=false;try{p->view.Measure(100,50);}catch(const std::runtime_error&){caught=true;}r.Truth("exception-propagated",caught);r.Size("last-completed",p->view.GetMeasuredSize(),MeasuredSize(37,19));p->onMeasure={};p->width=53;r.Size("retry",p->view.Measure(100,50),MeasuredSize(53,19)); }));
    out.push_back(Single("K18.arrange-throw", "Failed publication preserves the completed record and permits a cacheable retry", 19, [](Run& r)
    {
      auto p = Probed(r);
      p->view.Measure(100, 50);
      p->view.Arrange(LayoutRect(1, 2, 37, 19));
      p->onArrange = []()
      { throw std::runtime_error("LR30 expected arrange exception"); };
      bool caught = false;
      try
      {
        p->view.Arrange(LayoutRect(7, 9, 53, 29));
      }
      catch(const std::runtime_error&)
      {
        caught = true;
      }
      r.Truth("exception-propagated", caught);
      // The provisional actor geometry is visible before the producer runs.
      // Publication is the separate completed arrangement record.
      r.Rect("provisional-input", p->view, LayoutRect(7, 9, 53, 29));
      r.Rect("last-completed", GetImpl(p->view).GetArrangedBounds(), LayoutRect(1, 2, 37, 19));
      p->onArrange = {};
      r.Rect("retry", p->view.Arrange(LayoutRect(7, 9, 53, 29)), LayoutRect(7, 9, 53, 29));
      r.Equal("retry-producer", p->arranges, 3);
      r.Rect("retry-cache", p->view.Arrange(LayoutRect(7, 9, 53, 29)), LayoutRect(7, 9, 53, 29));
      r.Equal("retry-cache-producer", p->arranges, 3);
    }));
    out.push_back(Single("K18.measure-reentry", "Reentry terminates and does not publish a reusable poisoned result", 4, [](Run& r)
    {auto p=Probed(r);p->view.Measure(100,50);p->view.InvalidateMeasure();bool entered=false,caught=false;MeasuredSize nested;p->onMeasure=[p,&entered,&nested](){if(!entered){entered=true;nested=p->view.Measure(100,50);}};try{p->view.Measure(100,50);}catch(const DaliException&){caught=true;}p->onMeasure={};r.Truth("entered",entered);r.Truth("bounded",p->measures<=2);p->view.InvalidateMeasure();r.Size("recovered",p->view.Measure(100,50),MeasuredSize(37,19));(void)caught; }));
    return out;
  }
};

REGISTER_MANUAL_TEST(TcLr30)
