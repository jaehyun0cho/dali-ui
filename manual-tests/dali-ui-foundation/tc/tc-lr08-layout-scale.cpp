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

class TcLr08 : public Case
{
public:
  TcLr08()
  : Case("LR08", "Scale inheritance")
  {
  }
  std::vector<Scenario> BuildScenarios() override
  {
    std::vector<Scenario> out;
    for(float scale : {1.0f, 1.25f, 1.5f, 2.0f}) out.push_back(Single(("C13.scale." + std::to_string(scale)).c_str(), "Inherited enabled and disabled scale policies", 6, [scale](Run& r)
    {
      auto manager = UiScaleManager::Get();
      manager.SetScalable(true);
      manager.SetScale(scale);
      View p = Leaf(200, 100), a = Leaf(20, 10), b = Leaf(30, 15);
      p.SetUiScalePolicy(UiScalePolicy::DISABLED);
      a.SetUiScalePolicy(UiScalePolicy::INHERIT);
      b.SetUiScalePolicy(UiScalePolicy::ENABLED);
      p.Add(a);
      p.Add(b);
      p.Measure(400, 200);
      r.Size("parent-disabled", p.GetMeasuredSize(), MeasuredSize(200, 100));
      r.Size("child-inherit", a.GetMeasuredSize(), MeasuredSize(20, 10));
      r.Size("child-enabled", b.GetMeasuredSize(), MeasuredSize(30 * scale, 15 * scale));
    }));
    out.push_back(Single("C14.K12.master-switch", "Stored scale survives disable and re-enable", 6, [](Run& r)
    {
      auto m = UiScaleManager::Get();
      m.SetScalable(true);
      m.SetScale(2);
      View v = Leaf(37, 19);
      v.SetUiScalePolicy(UiScalePolicy::ENABLED);
      r.Attach(v);
      r.Size("enabled", v.Measure(200, 100), MeasuredSize(74, 38));
      m.SetScalable(false);
      r.Size("disabled", v.Measure(200, 100), MeasuredSize(37, 19));
      m.SetScalable(true);
      r.Size("restored", v.Measure(200, 100), MeasuredSize(74, 38));
    }));
    out.push_back(Single("R03.scale-finite", "Scale rejects nonfinite values without propagating them", 3, [](Run& r)
    {
      auto m = UiScaleManager::Get();
      m.SetScalable(false);
      m.SetScale(1);
      m.SetScale(std::numeric_limits<float>::quiet_NaN());
      r.Near("nan-rejected", m.GetScale(), 1, 0);
      m.SetScale(-1);
      r.Near("negative-rejected", m.GetScale(), 1, 0);
      m.SetScale(std::numeric_limits<float>::infinity());
      float observed = m.GetScale();
      m.SetScale(1);
      r.Near("infinity-rejected", observed, 1, 0);
    }));

    out.push_back(Single("C20.scale-exact-key", "Sub-epsilon scale changes invalidate a registered root", 5, [](Run& r)
    {
      if(!r.RequireDiagnostics()) return;
#if defined(DALI_UI_LAYOUT_TEST_DIAGNOSTICS)
      auto manager = UiScaleManager::Get();
      manager.SetScalable(true);
      manager.SetScale(1);
      auto p = Probed(r);
      p->view.SetUiScalePolicy(UiScalePolicy::ENABLED);
      r.Attach(p->view);
      p->view.Measure(128, 100);
      auto n = p->measures;
      manager.SetScale(1.0005f);
      p->view.Measure(128, 100);
      auto snapshot = Diagnostic::GetViewSnapshot(p->view);
      r.Equal("producer-recomputed", p->measures, n + 1);
      r.Truth("key-valid", snapshot.measureCacheValid);
      r.Near("exact-key", snapshot.measureScaleKey, 1.0005f, 0);
      r.Near("exact-effective", snapshot.effectiveScale, 1.0005f, 0);
      p->view.Measure(128, 100);
      r.Equal("same-scale-hit", p->measures, n + 1);
#endif
    }));
    out.push_back(Single("C13.actor-scale-repair", "Cached measure repairs framework-owned scale property", 5, [](Run& r)
    {
      if(!r.RequireDiagnostics()) return;
#if defined(DALI_UI_LAYOUT_TEST_DIAGNOSTICS)
      auto p = Probed(r);
      p->view.Measure(100, 50);
      auto before = Diagnostic::GetViewSnapshot(p->view);
      r.Truth("property-index", before.effectiveScalePropertyIndex != Property::INVALID_INDEX);
      Actor raw = p->view;
      raw.SetProperty(before.effectiveScalePropertyIndex, 9.0f);
      auto dirty = Diagnostic::GetViewSnapshot(p->view);
      r.Truth("sync-invalidated", !dirty.effectiveScaleActorSynced);
      p->view.Measure(100, 50);
      auto repaired = Diagnostic::GetViewSnapshot(p->view);
      r.Near("actor-scale", repaired.actorEffectiveScale, 1, 0);
      r.Truth("sync-restored", repaired.effectiveScaleActorSynced);
      r.Equal("producer-hit", p->measures, 1);
#endif
    }));

    return out;
  }
};

REGISTER_MANUAL_TEST(TcLr08)
