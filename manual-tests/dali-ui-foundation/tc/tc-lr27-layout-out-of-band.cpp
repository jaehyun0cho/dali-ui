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

class TcLr27 : public Case
{
public:
  TcLr27()
  : Case("LR27", "Out of band layout")
  {
  }
  std::vector<Scenario> BuildScenarios() override
  {
    std::vector<Scenario> out;
    out.push_back(Single("K14.external-measure", "Out-of-band Measure clears ancestor caches without scheduling dirty work", 19, [](Run& r)
    {
      if(!r.RequireDiagnostics()) return;
#if defined(DALI_UI_LAYOUT_TEST_DIAGNOSTICS)
      auto parent = StackLayout::New(StackOrientation::HORIZONTAL);
      Fixed(parent, 200, 80);
      auto p = Probed(r, 30, 10, false);
      p->view.SetRequestedWidth(MATCH_PARENT);
      p->view.SetRequestedHeight(MATCH_PARENT);
      parent.Add(p->view);
      Trace t;
      r.Truth("capture", t.Begin({parent, p->view}));
      Compute(parent, 200, 80);
      const auto before = Diagnostic::GetViewSnapshot(parent);
      p->view.Measure(70, 40);
      const auto external = Diagnostic::GetViewSnapshot(p->view);
      const auto owner    = Diagnostic::GetViewSnapshot(parent);
      Compute(parent, 200, 80);
      const auto restored = Diagnostic::GetViewSnapshot(p->view);
      const auto repaired = Diagnostic::GetViewSnapshot(parent);
      t.End();
      r.Truth("before.measure-cache", before.measureCacheValid);
      r.Truth("before.arrange-cache", before.arrangeCacheValid);
      r.Truth("external.owner-measure-cache-cleared", !owner.measureCacheValid);
      r.Truth("external.owner-arrange-cache-cleared", !owner.arrangeCacheValid);
      r.Truth("external.no-measure-dirty", !owner.measureDirty);
      r.Truth("external.no-arrange-dirty", !owner.arrangeDirty);
      r.Truth("external.no-poison", !owner.measurePoisoned && !owner.arrangePoisoned);
      r.Size("external.constraint", external.normalizedConstraint, MeasuredSize(70, 40));
      r.Size("restored.constraint", restored.normalizedConstraint, MeasuredSize(200, 80));
      r.Truth("restored.measure-cache", repaired.measureCacheValid);
      r.Truth("restored.arrange-cache", repaired.arrangeCacheValid);
      r.Rect("restored", p->view, LayoutRect(0, 0, 200, 80));
      r.Truth("no-overflow", !t.result.overflow);
#endif
    }));
    out.push_back(Single("K14.external-standalone-measure", "Standalone slot correction is consumed by parent Arrange even after a parent Measure hit", 16, [](Run& r)
    {
      if(!r.RequireDiagnostics()) return;
#if defined(DALI_UI_LAYOUT_TEST_DIAGNOSTICS)
      auto parent = StackLayout::New(StackOrientation::HORIZONTAL);
      Fixed(parent, 200, 80);
      auto p = Probed(r, 300, 90, false);
      p->view.SetLayoutMode(LayoutMode::STANDALONE);
      p->view.SetRequestedX(7);
      p->view.SetRequestedY(9);
      parent.Add(p->view);
      Trace t;
      r.Truth("capture", t.Begin({parent, p->view}));
      Compute(parent, 200, 80);
      const auto before = Diagnostic::GetViewSnapshot(parent);
      r.Truth("before.parent-cache", before.measureCacheValid && before.arrangeCacheValid);
      r.Truth("before.standalone-slot-consumed", !Diagnostic::GetViewSnapshot(p->view).measuredSlotUnconsumed);
      const auto measures = p->measures;
      r.Size("external.measure-result", p->view.Measure(70, 40), MeasuredSize(70, 40));
      r.Truth("external.standalone-slot-unconsumed", Diagnostic::GetViewSnapshot(p->view).measuredSlotUnconsumed);
      const auto owner = Diagnostic::GetViewSnapshot(parent);
      r.Truth("external.parent-cache-preserved", owner.measureCacheValid && owner.arrangeCacheValid);
      parent.Measure(200, 80);
      r.Equal("parent-measure-hit-no-child-producer", p->measures, measures + 1);
      parent.Arrange(LayoutRect(0, 0, 200, 80));
      r.Truth("corrected.slot-consumed", !Diagnostic::GetViewSnapshot(p->view).measuredSlotUnconsumed);
      r.Equal("corrective-child-producer", p->measures, measures + 2);
      r.Rect("corrected.slot", p->view, LayoutRect(7, 9, 200, 80));
      parent.Arrange(LayoutRect(0, 0, 200, 80));
      r.Equal("repeat.no-child-producer", p->measures, measures + 2);
      t.End();
      r.Truth("no-overflow", !t.result.overflow);
#endif
    }));
    out.push_back(Single("K14.external-arrange", "Direct child Arrange cannot leave stale ancestor replay", 8, [](Run& r)
    {auto parent=StackLayout::New(StackOrientation::HORIZONTAL);Fixed(parent,100,50);View a=Leaf(20,10),b=Leaf(30,10);parent.Add(a);parent.Add(b);Compute(parent,100,50);b.Arrange(LayoutRect(75,20,10,5));r.Rect("external",b,LayoutRect(75,20,10,5));parent.Arrange(LayoutRect(0,0,100,50));r.Rect("restored",b,LayoutRect(20,0,30,10)); }));
    return out;
  }
};

REGISTER_MANUAL_TEST(TcLr27)
