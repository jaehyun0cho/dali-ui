/* Copyright (c) 2026 Samsung Electronics Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 */
#include <dali-ui-foundation/public-api/views/view-impl.h>
#include <vector>
#include "layout-validation-transition-fixtures.h"
using namespace LayoutValidation;
using namespace LayoutValidation::TransitionFixtures;
class Tc139 : public Case
{
public:
  Tc139()
  : Case("TC139", "Controller batch and root ordering")
  {
  }
  std::vector<Scenario> BuildScenarios() override
  {
    std::vector<Scenario> cases;
    cases = {
      {"CT02.depth-and-dedup", "Duplicate queued roots are consumed once in ancestor-before-descendant order", {{"mount", 4, [](Run& r)
    {
      auto s = NewState(r);
      s->child.SetLayoutMode(LayoutMode::STANDALONE);
      s->child.SetRequestedX(40);
      s->child.SetRequestedY(30);
      Mount(r, s);
    }},
                                                                                                                {"queue-reverse-order", 10, [](Run& r)
    {
      auto s = r.State<State>();
      Watch(s);
      std::array<Diagnostics::Event, 4096> events{};
      CaptureScope                         captureScope(events.data(), events.size(), 4601);
      auto&                                controller = LayoutController::Get(s->window);
      s->root.InvalidateMeasure();
      s->child.InvalidateMeasure();
      controller.RequestLayout(&GetImpl(s->child));
      controller.RequestLayout(&GetImpl(s->root));
      controller.RequestLayout(&GetImpl(s->child));
      controller.ProcessLayouts();
      const auto  capture = captureScope.Finish();
      std::size_t root = events.size(), child = events.size();
      uint32_t    rootDrains = 0, childDrains = 0;
      for(std::size_t i = 0; i < capture.count; ++i)
        if(events[i].kind == Diagnostics::EventKind::ROOT_DRAIN)
        {
          if(events[i].nodeId == 1)
          {
            root = std::min(root, i);
            ++rootDrains;
          }
          if(events[i].nodeId == 2)
          {
            child = std::min(child, i);
            ++childDrains;
          }
        }
      r.Truth("batch.capture.complete", !capture.overflow);
      r.Truth("batch.root.present", root < events.size());
      r.Truth("batch.child.present", child < events.size());
      r.Truth("batch.ancestor.first", root < child);
      r.Equal("batch.root.once", rootDrains, 1);
      r.Equal("batch.child.once", childDrains, 1);
      r.Rect("batch.child.target", Diagnostics::GetViewSnapshot(s->child).arrangedBounds, {40, 30, 80, 40});
    }}}},
      {"CT02.deleted-pending-root", "Unregistering a queued off-scene root removes pending work without dangling access", {{"unregister", 4, [](Run& r)
    {
      auto  s          = NewState(r);
      auto& controller = LayoutController::Get(s->window);
      s->root          = View::New();
      s->root.SetMeasureCallback(MeasureCallback::New(s.get(), &State::Measure));
      const auto before = Diagnostics::GetWindowSnapshot(s->window).pendingRoots;
      controller.RequestLayout(&GetImpl(s->root));
      r.Equal("unregister.pending.armed", Diagnostics::GetWindowSnapshot(s->window).pendingRoots, before + 1);
      controller.UnregisterView(&GetImpl(s->root));
      r.Equal("unregister.pending", Diagnostics::GetWindowSnapshot(s->window).pendingRoots, before);
      controller.ProcessLayouts();
      r.Truth("unregister.controller.live", Diagnostics::GetWindowSnapshot(s->window).valid);
      r.Equal("unregister.no-producer", s->measureCalls, 0);
    }}}}};
    // A standalone child of the vertical 300x200 stack root queues its own turn, then switches to
    // DEFAULT in the same batch: the switch withdraws its root registration and the queued turn is
    // never drained; the stack lays it out after the 20-high sibling.
    cases.push_back({"CT02.mode-switch-cancels-self-turn", "Switching a standalone child to DEFAULT releases its root registration and cancels its queued own turn", {{"mount", 8, [](Run& r)
    {
      auto s = NewState(r, true);
      s->child.SetLayoutMode(LayoutMode::STANDALONE);
      s->child.SetRequestedX(40);
      s->child.SetRequestedY(30);
      s->root.Add(s->sibling);
      s->root.Add(s->child);
      r.Attach(s->root);
      r.AfterLayout({s->child, s->sibling}, [s](Run& v)
      {
        v.Rect("switch.standalone.target", v.Snapshot(s->child), {40, 30, 80, 40});
        v.Rect("switch.sibling.target", v.Snapshot(s->sibling), {0, 0, 60, 20});
      });
    }},
                                                                                                                                                                                                {"switch-before-turn", 12, [](Run& r)
    {
      auto s = r.State<State>();
      Watch(s);
      std::vector<Diagnostics::Event> events(16384);
      CaptureScope                    captureScope(events.data(), events.size(), 4602);
      const auto                      p0 = Diagnostics::GetWindowSnapshot(s->window).pendingRoots;
      s->child.InvalidateMeasure();
      const auto queued = Diagnostics::GetWindowSnapshot(s->window);
      s->child.SetLayoutMode(LayoutMode::DEFAULT);
      const auto withdrawn = Diagnostics::GetWindowSnapshot(s->window);
      LayoutController::Get(s->window).ProcessLayouts();
      const auto capture = captureScope.Finish();
      uint32_t   drains  = 0;
      for(std::size_t i = 0; i < capture.count && i < events.size(); ++i)
        if(events[i].kind == Diagnostics::EventKind::ROOT_DRAIN && events[i].nodeId == 2) ++drains;
      r.Equal("switch.self-turn.queued", static_cast<int64_t>(queued.pendingRoots) - static_cast<int64_t>(p0), 1);
      r.Equal("switch.registration.withdrawn", static_cast<int64_t>(queued.registeredRoots) - static_cast<int64_t>(withdrawn.registeredRoots), 1);
      r.Equal("switch.cancelled-turn.drains", drains, 0);
      r.Truth("switch.trace.complete", !capture.overflow);
      r.Rect("switch.default.target", LayoutValidation::Bounds(s->child), {0, 20, 80, 40});
      r.Rect("switch.default.logical", Diagnostics::GetViewSnapshot(s->child).arrangedBounds, {0, 20, 80, 40});
    }},
                                                                                                                                                                                                {"steady", 4, [](Run& r)
    {
      auto s = r.State<State>();
      r.AfterFrame([s](Run& v)
      { v.Rect("switch.steady.current", Current(s->child), {0, 20, 80, 40}); }, {}, s->child);
    }}}});
    return cases;
  }
};
REGISTER_MANUAL_TEST(Tc139)
