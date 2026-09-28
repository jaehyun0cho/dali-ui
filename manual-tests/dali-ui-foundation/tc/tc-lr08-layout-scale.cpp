/* Copyright (c) 2026 Samsung Electronics Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 */
#include "layout-validation-fixtures.h"

using namespace LayoutValidation;
using namespace LayoutValidation::Fixtures;

namespace
{
struct Scaled
{
  View stage, view, sibling;
  float nudge{10};
};
struct ScaledProbe
{
  std::shared_ptr<Probe> probe;
  View                   stage, sibling;
  uint32_t               measures{0};
  float                  nudge{10};
};
// Stages for scale scenarios opt out of scaling so their pixel size stays the declared size.
View UnscaledStage(Run& r, float width, float height)
{
  View stage = r.Stage(width, height);
  stage.SetUiScalePolicy(UiScalePolicy::DISABLED);
  return stage;
}
} // namespace

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
    for(float scale : {1.0f, 1.25f, 1.5f, 2.0f})
      out.push_back(Single(("C13.scale." + FloatId(scale)).c_str(), "Inherited enabled and disabled scale policies render at their own scale", 14, [scale](Run& r) {
        auto manager = UiScaleManager::Get();
        manager.SetScalable(true);
        manager.SetScale(scale);
        View p = Leaf(200, 100, "parent"), a = Leaf(20, 10, "inherit"), b = Leaf(30, 15, "enabled");
        p.SetUiScalePolicy(UiScalePolicy::DISABLED);
        a.SetUiScalePolicy(UiScalePolicy::INHERIT);
        b.SetUiScalePolicy(UiScalePolicy::ENABLED);
        p.Add(a);
        p.Add(b);
        View stage = UnscaledStage(r, 400, 200);
        stage.Add(p);
        r.AfterRender({stage, p, a, b}, [=](Run& r) {
          r.Size("parent-disabled", p.GetMeasuredSize(), MeasuredSize(200, 100));
          r.Size("child-inherit", a.GetMeasuredSize(), MeasuredSize(20, 10));
          r.Size("child-enabled", b.GetMeasuredSize(), MeasuredSize(30 * scale, 15 * scale));
          r.Rendered("child-inherit.rendered", a, LayoutRect(0, 0, 20, 10));
          r.Rendered("child-enabled.rendered", b, LayoutRect(0, 0, 30 * scale, 15 * scale));
        });
      }));
    out.push_back(Steps("C14.K12.master-switch", "Stored scale survives disable and re-enable on screen", {
      {"enabled", 6, [](Run& r) {
         auto m = UiScaleManager::Get();
         m.SetScalable(true);
         m.SetScale(2);
         auto s  = std::make_shared<Scaled>();
         s->view = Leaf(37, 19);
         s->view.SetUiScalePolicy(UiScalePolicy::ENABLED);
         s->stage = UnscaledStage(r, 200, 100);
         s->stage.Add(s->view);
         r.SetState(s);
         r.AfterRender({s->stage, s->view}, [=](Run& r) {
           r.Size("enabled", s->view.GetMeasuredSize(), MeasuredSize(74, 38));
           r.Rendered("enabled.rendered", s->view, LayoutRect(0, 0, 74, 38));
         });
       }},
      {"disabled", 6, [](Run& r) {
         auto s = r.State<Scaled>();
         UiScaleManager::Get().SetScalable(false);
         s->view.InvalidateMeasure();
         r.AfterRender({s->stage, s->view}, [=](Run& r) {
           r.Size("disabled", s->view.GetMeasuredSize(), MeasuredSize(37, 19));
           r.Rendered("disabled.rendered", s->view, LayoutRect(0, 0, 37, 19));
         });
       }},
      {"restored", 6, [](Run& r) {
         auto s = r.State<Scaled>();
         UiScaleManager::Get().SetScalable(true);
         s->view.InvalidateMeasure();
         r.AfterRender({s->stage, s->view}, [=](Run& r) {
           r.Size("restored", s->view.GetMeasuredSize(), MeasuredSize(74, 38));
           r.Rendered("restored.rendered", s->view, LayoutRect(0, 0, 74, 38));
         });
       }},
    }));
    out.push_back(Single("R03.scale-finite", "Scale rejects nonfinite values without propagating them", 4, [](Run& r) {
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
      // A subnormal is finite and positive but unusable: the layout divides by the effective
      // scale, and 1 / denorm_min overflows to infinity.
      m.SetScale(std::numeric_limits<float>::denorm_min());
      r.Near("subnormal-rejected", m.GetScale(), 1, 0);
      m.SetScale(1);
    }));
    out.push_back(Steps("C20.scale-exact-key", "Sub-epsilon scale changes invalidate a registered root and keep an exact cache key", {
      {"mount", 4, [](Run& r) {
         auto manager = UiScaleManager::Get();
         manager.SetScalable(true);
         manager.SetScale(1);
         auto s     = std::make_shared<ScaledProbe>();
         s->probe   = Probed(r);
         s->sibling = Leaf(10, 10, "sibling");
         s->sibling.SetRequestedY(60);
         s->probe->view.SetUiScalePolicy(UiScalePolicy::ENABLED);
         s->stage = UnscaledStage(r, 128, 100);
         s->stage.Add(s->probe->view);
         s->stage.Add(s->sibling);
         r.SetState(s);
         r.AfterRender({s->stage, s->probe->view, s->sibling}, [=](Run& r) {
           s->measures = s->probe->measures;
           r.Rendered("probe", s->probe->view, LayoutRect(0, 0, 37, 19));
         });
       }},
      {"sub-epsilon-scale", 4, [](Run& r) {
         auto s = r.State<ScaledProbe>();
         UiScaleManager::Get().SetScale(1.0005f);
         r.AfterRender({s->stage, s->probe->view, s->sibling}, [=](Run& r) {
           auto snapshot = Diagnostic::GetViewSnapshot(s->probe->view);
           r.Equal("producer-recomputed", s->probe->measures, s->measures + 1);
           r.Truth("key-valid", snapshot.measureCacheValid);
           r.Near("exact-key", snapshot.measureScaleKey, 1.0005f, 0);
           r.Near("exact-effective", snapshot.effectiveScale, 1.0005f, 0);
           s->measures = s->probe->measures;
         });
       }},
      {"same-scale", 1, [](Run& r) {
         auto s = r.State<ScaledProbe>();
         s->sibling.SetRequestedWidth(s->nudge += 1);
         r.AfterRender({s->stage, s->probe->view, s->sibling}, [=](Run& r) { r.Equal("same-scale-hit", s->probe->measures, s->measures); });
       }},
    }));
    out.push_back(Steps("C13.actor-scale-repair", "A cached measure repairs the framework-owned scale property", {
      {"mount", 5, [](Run& r) {
         auto s     = std::make_shared<ScaledProbe>();
         s->probe   = Probed(r);
         s->sibling = Leaf(10, 10, "sibling");
         s->sibling.SetRequestedY(60);
         s->stage = UnscaledStage(r, 100, 100);
         s->stage.Add(s->probe->view);
         s->stage.Add(s->sibling);
         r.SetState(s);
         r.AfterRender({s->stage, s->probe->view, s->sibling}, [=](Run& r) {
           auto before = Diagnostic::GetViewSnapshot(s->probe->view);
           r.Truth("property-index", before.effectiveScalePropertyIndex != Property::INVALID_INDEX);
           r.Rendered("probe", s->probe->view, LayoutRect(0, 0, 37, 19));
           s->measures = s->probe->measures;
         });
       }},
      {"corrupt-and-repair", 4, [](Run& r) {
         auto s      = r.State<ScaledProbe>();
         auto before = Diagnostic::GetViewSnapshot(s->probe->view);
         Actor raw   = s->probe->view;
         raw.SetProperty(before.effectiveScalePropertyIndex, 9.0f);
         r.Truth("sync-invalidated", !Diagnostic::GetViewSnapshot(s->probe->view).effectiveScaleActorSynced);
         s->sibling.SetRequestedWidth(s->nudge += 1);
         r.AfterRender({s->stage, s->probe->view, s->sibling}, [=](Run& r) {
           auto repaired = Diagnostic::GetViewSnapshot(s->probe->view);
           r.Near("actor-scale", repaired.actorEffectiveScale, 1, 0);
           r.Truth("sync-restored", repaired.effectiveScaleActorSynced);
           r.Equal("producer-hit", s->probe->measures, s->measures);
         });
       }},
    }));
    return out;
  }
};

REGISTER_MANUAL_TEST(TcLr08)
