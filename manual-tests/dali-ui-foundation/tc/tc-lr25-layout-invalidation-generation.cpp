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

class TcLr25 : public Case
{
public:
  TcLr25()
  : Case("LR25", "Invalidation generation")
  {
  }
  std::vector<Scenario> BuildScenarios() override
  {
    std::vector<Scenario> out;
    out.push_back(Single("K06.generation", "Outer operation generations and dirty propagation", 9, [](Run& r)
    {
      if(!r.RequireDiagnostics()) return;
#if defined(DALI_UI_LAYOUT_TEST_DIAGNOSTICS)
      auto root  = StackLayout::New(StackOrientation::HORIZONTAL);
      auto child = Probed(r, 20, 10, false);
      root.Add(child->view);
      Trace t;
      r.Truth("capture", t.Begin({root, child->view}));
      root.Measure(100, 50);
      auto before = Diagnostic::GetViewSnapshot(root);
      child->view.InvalidateMeasure();
      auto dirty = Diagnostic::GetViewSnapshot(root);
      root.Measure(100, 50);
      auto after = Diagnostic::GetViewSnapshot(root);
      t.End();
      r.Truth("valid", before.valid && dirty.valid && after.valid);
      r.Truth("initial-cache", before.measureCacheValid);
      r.Truth("propagated-dirty", dirty.measureDirty);
      r.Truth("cache-invalidated", !dirty.measureCacheValid);
      r.Truth("new-generation", after.generation != before.generation);
      r.Truth("new-cache", after.measureCacheValid && !after.measureDirty);
      r.Truth("ancestor-visited", t.Count(Diagnostic::EventKind::ANCESTOR_VISIT) > 0);
      r.Truth("no-overflow", !t.result.overflow);
#endif
    }));
    out.push_back(Single("K07.arrange-only", "Arrange invalidation preserves measure cache", 7, [](Run& r)
    {
      if(!r.RequireDiagnostics()) return;
#if defined(DALI_UI_LAYOUT_TEST_DIAGNOSTICS)
      auto p = Probed(r);
      p->view.Measure(100, 50);
      p->view.Arrange(LayoutRect(0, 0, 37, 19));
      auto before = Diagnostic::GetViewSnapshot(p->view);
      p->view.InvalidateArrange();
      auto dirty = Diagnostic::GetViewSnapshot(p->view);
      p->view.Arrange(LayoutRect(0, 0, 37, 19));
      auto after = Diagnostic::GetViewSnapshot(p->view);
      r.Truth("valid", before.valid && dirty.valid && after.valid);
      r.Truth("measure-preserved", dirty.measureCacheValid);
      r.Truth("arrange-invalid", !dirty.arrangeCacheValid);
      r.Truth("dirty", dirty.arrangeDirty);
      r.Truth("settled", after.arrangeCacheValid && !after.arrangeDirty);
      r.Equal("measure-producer", p->measures, 1);
      r.Equal("arrange-producer", p->arranges, 2);
#endif
    }));
    return out;
  }
};

REGISTER_MANUAL_TEST(TcLr25)
