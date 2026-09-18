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
struct OutOfBand
{
  std::shared_ptr<Probe> probe;
  View                   stage, parent, sibling, a, b;
  uint32_t               measures{0};
  float                  nudge{10};
  void                   Nudge()
  {
    sibling.SetRequestedWidth(nudge += 1);
  }
  std::shared_ptr<Trace> trace;
};
// An on-screen horizontal stack (200x80) holding the probe, next to a sibling leaf used to
// force later controller passes without touching the stack subtree.
std::shared_ptr<OutOfBand> MountStack(Run& r, std::shared_ptr<Probe> probe)
{
  auto s     = std::make_shared<OutOfBand>();
  s->probe   = probe;
  s->parent  = StackLayout::New(StackOrientation::HORIZONTAL);
  Fixed(s->parent, 200, 80);
  s->parent.Add(probe->view);
  s->sibling = Leaf(10, 10, "sibling");
  s->sibling.SetRequestedY(90);
  s->stage = r.Stage(200, 110);
  s->stage.Add(s->parent);
  s->stage.Add(s->sibling);
  r.SetState(s);
  return s;
}
} // namespace

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
    out.push_back(Steps("K14.external-measure", "An out-of-band Measure on a rendered tree clears ancestor caches without scheduling work; the next pass restores the slot", {
      {"mount", 6, [](Run& r) {
         auto probe = Probed(r, 30, 10, false);
         probe->view.SetRequestedWidth(MATCH_PARENT);
         probe->view.SetRequestedHeight(MATCH_PARENT);
         auto s = MountStack(r, probe);
         r.AfterRender({s->stage, s->parent, s->probe->view, s->sibling}, [=](Run& r) {
           const auto before = Diagnostic::GetViewSnapshot(s->parent);
           r.Truth("before.measure-cache", before.measureCacheValid);
           r.Truth("before.arrange-cache", before.arrangeCacheValid);
           r.Rendered("before.slot", s->probe->view, LayoutRect(0, 0, 200, 80));
         });
       }},
      {"external", 17, [](Run& r) {
         auto s   = r.State<OutOfBand>();
         s->trace = std::make_shared<Trace>();
         r.Truth("capture", s->trace->Begin({s->parent, s->probe->view}));
         s->probe->view.Measure(70, 40);
         const auto external = Diagnostic::GetViewSnapshot(s->probe->view);
         const auto owner    = Diagnostic::GetViewSnapshot(s->parent);
         r.Truth("external.owner-measure-cache-cleared", !owner.measureCacheValid);
         r.Truth("external.owner-arrange-cache-cleared", !owner.arrangeCacheValid);
         r.Truth("external.no-measure-dirty", !owner.measureDirty);
         r.Truth("external.no-arrange-dirty", !owner.arrangeDirty);
         r.Truth("external.no-poison", !owner.measurePoisoned && !owner.arrangePoisoned);
         r.Size("external.constraint", external.normalizedConstraint, MeasuredSize(70, 40));
         s->Nudge();
         r.AfterRender({s->stage, s->parent, s->probe->view, s->sibling}, [=](Run& r) {
           const auto restored = Diagnostic::GetViewSnapshot(s->probe->view);
           const auto repaired = Diagnostic::GetViewSnapshot(s->parent);
           s->trace->End();
           r.Size("restored.constraint", restored.normalizedConstraint, MeasuredSize(200, 80));
           r.Truth("restored.measure-cache", repaired.measureCacheValid);
           r.Truth("restored.arrange-cache", repaired.arrangeCacheValid);
           r.Rendered("restored", s->probe->view, LayoutRect(0, 0, 200, 80));
           r.Truth("no-overflow", !s->trace->result.overflow);
         });
       }},
    }));
    out.push_back(Steps("K14.external-standalone-measure", "A standalone slot corrected out of band is consumed by the parent's next Arrange", {
      {"mount", 6, [](Run& r) {
         auto probe = Probed(r, 300, 90, false);
         probe->view.SetLayoutMode(LayoutMode::STANDALONE);
         probe->view.SetRequestedX(7);
         probe->view.SetRequestedY(9);
         auto s = MountStack(r, probe);
         r.AfterRender({s->stage, s->parent, s->sibling}, [=](Run& r) {
           const auto before = Diagnostic::GetViewSnapshot(s->parent);
           s->measures       = s->probe->measures;
           r.Truth("before.parent-cache", before.measureCacheValid && before.arrangeCacheValid);
           r.Truth("before.standalone-slot-consumed", !Diagnostic::GetViewSnapshot(s->probe->view).measuredSlotUnconsumed);
           r.Rendered("before.slot", s->probe->view, LayoutRect(7, 9, 200, 80));
         });
       }},
      {"external", 11, [](Run& r) {
         auto s   = r.State<OutOfBand>();
         s->trace = std::make_shared<Trace>();
         r.Truth("capture", s->trace->Begin({s->parent, s->probe->view}));
         r.Size("external.measure-result", s->probe->view.Measure(70, 40), MeasuredSize(70, 40));
         r.Truth("external.standalone-slot-unconsumed", Diagnostic::GetViewSnapshot(s->probe->view).measuredSlotUnconsumed);
         const auto owner = Diagnostic::GetViewSnapshot(s->parent);
         r.Truth("external.parent-cache-preserved", owner.measureCacheValid && owner.arrangeCacheValid);
         s->Nudge();
         r.AfterRender({s->stage, s->parent, s->sibling}, [=](Run& r) {
           r.Truth("corrected.slot-consumed", !Diagnostic::GetViewSnapshot(s->probe->view).measuredSlotUnconsumed);
           r.Equal("corrective-child-producer", s->probe->measures, s->measures + 2);
           r.Rendered("corrected.slot", s->probe->view, LayoutRect(7, 9, 200, 80));
         });
       }},
      {"repeat", 2, [](Run& r) {
         auto s = r.State<OutOfBand>();
         s->Nudge();
         r.AfterRender({s->stage, s->parent, s->sibling}, [=](Run& r) {
           r.Equal("repeat.no-child-producer", s->probe->measures, s->measures + 2);
           s->trace->End();
           r.Truth("no-overflow", !s->trace->result.overflow);
         });
       }},
    }));
    out.push_back(Steps("K14.external-arrange", "A direct child Arrange on a rendered tree cannot leave a stale ancestor replay", {
      {"mount", 8, [](Run& r) {
         auto s    = std::make_shared<OutOfBand>();
         s->parent = StackLayout::New(StackOrientation::HORIZONTAL);
         Fixed(s->parent, 100, 50);
         s->a = Leaf(20, 10, "a");
         s->b = Leaf(30, 10, "b");
         s->parent.Add(s->a);
         s->parent.Add(s->b);
         s->sibling = Leaf(10, 10, "sibling");
         s->sibling.SetRequestedY(60);
         s->stage = r.Stage(100, 80);
         s->stage.Add(s->parent);
         s->stage.Add(s->sibling);
         r.SetState(s);
         r.AfterRender({s->stage, s->parent, s->a, s->b, s->sibling}, [=](Run& r) {
           r.Rendered("a", s->a, LayoutRect(0, 0, 20, 10));
           r.Rendered("b", s->b, LayoutRect(20, 0, 30, 10));
         });
       }},
      {"external-arrange", 8, [](Run& r) {
         auto s = r.State<OutOfBand>();
         s->b.Arrange(LayoutRect(75, 20, 10, 5));
         r.Rect("external", LayoutValidation::Bounds(s->b), LayoutRect(75, 20, 10, 5));
         s->Nudge();
         r.AfterRender({s->stage, s->parent, s->a, s->b, s->sibling}, [=](Run& r) { r.Rendered("restored", s->b, LayoutRect(20, 0, 30, 10)); });
       }},
    }));
    return out;
  }
};

REGISTER_MANUAL_TEST(TcLr27)
