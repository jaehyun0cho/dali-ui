/* Copyright (c) 2026 Samsung Electronics Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */
#include <dali-ui-foundation/public-api/views/view-impl.h>
#include "layout-validation-transition-fixtures.h"
using namespace LayoutValidation;
using namespace LayoutValidation::TransitionFixtures;
class TcLr46 : public Case
{
public:
  TcLr46()
  : Case("LR46", "Controller batch and root ordering")
  {
  }
  std::vector<Scenario> BuildScenarios() override
  {
    return {
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
      if(!r.RequireDiagnostics()) return;
#if defined(DALI_UI_LAYOUT_TEST_DIAGNOSTICS)
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
#endif
    }}}},
      {"CT02.deleted-pending-root", "Unregistering a queued off-scene root removes pending work without dangling access", {{"unregister", 3, [](Run& r)
    {
      if(!r.RequireDiagnostics()) return;
#if defined(DALI_UI_LAYOUT_TEST_DIAGNOSTICS)
      auto  s          = NewState(r);
      auto& controller = LayoutController::Get(s->window);
      s->root          = View::New();
      s->root.SetMeasureCallback(MeasureCallback::New(s.get(), &State::Measure));
      const auto before = Diagnostics::GetWindowSnapshot(s->window).pendingRoots;
      controller.RequestLayout(&GetImpl(s->root));
      controller.UnregisterView(&GetImpl(s->root));
      r.Equal("unregister.pending", Diagnostics::GetWindowSnapshot(s->window).pendingRoots, before);
      controller.ProcessLayouts();
      r.Truth("unregister.controller.live", Diagnostics::GetWindowSnapshot(s->window).valid);
      r.Equal("unregister.no-producer", s->measureCalls, 0);
#endif
    }}}}};
  }
};
REGISTER_MANUAL_TEST(TcLr46)
