/* Copyright (c) 2026 Samsung Electronics Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */
#include "layout-validation-transition-fixtures.h"
using namespace LayoutValidation;
using namespace LayoutValidation::TransitionFixtures;
class TcLr37 : public Case
{
public:
  TcLr37()
  : Case("LR37", "Transition CHANGE causes and precedence")
  {
  }
  std::vector<Scenario> BuildScenarios() override
  {
    std::vector<Scenario> cases;
    for(unsigned kind = 0; kind < 7; ++kind)
      cases.push_back({"TR03.cause-" + std::to_string(kind), "All five causes, resize opt-out and simultaneous cause precedence", {{"mount", 4, [](Run& r)
      {
        auto s = NewState(r, true);
        s->root.Add(s->sibling);
        Mount(r, s, {0, 20, 80, 40});
      }},
                                                                                                                                   {"change", 14, [kind](Run& r)
      {
        if(!r.RequireDiagnostics()) return;
#if defined(DALI_UI_LAYOUT_TEST_DIAGNOSTICS)
        auto s = r.State<State>();
        Diagnostics::SetManualAnimatorTicks(s->window, true);
        CaptureLifecycle(s);
        s->transition.SetChangeOnWindowResize(kind != 5);
        auto       cause = LayoutChangeCause::OTHER;
        LayoutRect target{0, 20, 120, 40};
        if(kind == 0 || kind == 6)
        {
          s->root.Add(s->other);
          cause = LayoutChangeCause::SIBLING_ADDED;
        }
        if(kind == 1 || kind == 6)
        {
          s->root.Remove(s->sibling);
          cause    = kind == 6 ? cause : LayoutChangeCause::SIBLING_REMOVED;
          target.y = 0;
        }
        if(kind == 2 || kind == 6)
        {
          if(kind == 2)
            s->child.LowerToBottom(LayoutOrderPolicy::UPDATE);
          else
            s->child.RaiseToTop(LayoutOrderPolicy::UPDATE);
          cause    = LayoutChangeCause::REORDERED;
          target.y = kind == 6 ? 120.f : 0.f;
        }
        s->child.SetRequestedWidth(120);
        if(kind == 3 || kind == 5)
        {
          const auto size   = s->window.GetPositionSize();
          const auto window = s->window;
          r.OnCleanup([window, size]()
          { LayoutController::Get(window).OnWindowResize(size.width, size.height); });
          LayoutController::Get(s->window).OnWindowResize(640, 480);
          cause = LayoutChangeCause::WINDOW_RESIZED;
        }
        LayoutController::Get(s->window).ProcessLayouts();
        Diagnostics::TickAnimatorsForTesting(s->window, 0);
        Observation found;
        bool        seen = false;
        for(std::size_t i = 0; i < s->observationCount; ++i)
          if(s->observations[i].viewId == 2)
          {
            found = s->observations[i];
            seen  = true;
            break;
          }
        if(kind == 5)
        {
          r.Truth("resize.opt-out.no-context", !seen);
          r.Truth("resize.opt-out.no-active", !Diagnostics::GetTransitionSnapshot(s->child).animatorActive);
          r.Rect("resize.opt-out.target", LayoutValidation::Bounds(s->child), target);
          r.Rect("resize.opt-out.logical", Diagnostics::GetViewSnapshot(s->child).arrangedBounds, target);
          r.Equal("resize.opt-out.start", s->starts[2], 0);
          r.Equal("resize.opt-out.finish", s->finishes[2], 0);
          r.Truth("resize.opt-out.capture", !s->observationOverflow);
          r.Truth("resize.opt-out.valid", Diagnostics::GetViewSnapshot(s->child).valid);
        }
        else
        {
          r.Truth("cause.context.present", seen);
          CheckObservation(r, found, LayoutTransitionSlot::CHANGE, cause, {0, 20, 80, 40}, target);
          r.Near("cause.first.raw", found.raw, 0);
          r.Truth("cause.capture.complete", !s->observationOverflow);
          r.Truth("cause.engine.active", Diagnostics::GetTransitionSnapshot(s->child).animatorActive);
        }
#endif
      }}}});
    for(float delta : {.49f, .5f, .51f})
      cases.push_back({"TR14.threshold-" + std::to_string(delta), "Half-pixel CHANGE threshold", {{"mount", 4, [](Run& r)
      { Mount(r, NewState(r)); }},
                                                                                                  {"threshold", 5, [delta](Run& r)
      {
        if(!r.RequireDiagnostics()) return;
#if defined(DALI_UI_LAYOUT_TEST_DIAGNOSTICS)
        auto s = r.State<State>();
        CaptureLifecycle(s);
        SetFixtureBounds(s->child, {40 + delta, 30, 80, 40});
        LayoutController::Get(s->window).ProcessLayouts();
        r.Rect("threshold.layout", Diagnostics::GetViewSnapshot(s->child).arrangedBounds, {40 + delta, 30, 80, 40});
        r.Equal("threshold.start", s->starts[2], delta > .5f ? 1 : 0);
#endif
      }}}});
    cases.push_back({"TR01.timing-enable-override", "Default disabled, cause override independent, clear and zero-duration settle", {{"mount", 4, [](Run& r)
    { Mount(r, NewState(r)); }},
                                                                                                                                     {"default-disabled", 5, [](Run& r)
    {
      auto s = r.State<State>();
      s->Bind(LayoutTransition::New());
      s->transition.ClearChangeTiming();
      s->root.SetLayoutTransition(s->transition);
      SetFixtureBounds(s->child, {80, 30, 80, 40});
      LayoutController::Get(s->window).ProcessLayouts();
      r.Rect("disabled.target", LayoutValidation::Bounds(s->child), {80, 30, 80, 40});
      r.Equal("disabled.start", s->starts[2], 0);
    }},
                                                                                                                                     {"override-enabled", 3, [](Run& r)
    {
      if(!r.RequireDiagnostics()) return;
#if defined(DALI_UI_LAYOUT_TEST_DIAGNOSTICS)
      auto s = r.State<State>();
      s->transition.SetChangeTiming(LayoutChangeCause::OTHER, Timing(.6f));
      SetFixtureBounds(s->child, {120, 30, 80, 40});
      LayoutController::Get(s->window).ProcessLayouts();
      s->animation = Diagnostics::GetSpecAnimation(s->child);
      r.Truth("override.active", static_cast<bool>(s->animation));
      if(s->animation)
      {
        s->animation.Pause();
        r.Near("override.duration", s->animation.GetDuration(), .6);
      }
      else
        r.Fail("override.duration", "missing actual animation");
      r.Equal("override.start", s->starts[2], 1);
#endif
    }},
                                                                                                                                     {"override-cleared", 6, [](Run& r)
    {
      auto s = r.State<State>();
      s->transition.ClearChangeTiming(LayoutChangeCause::OTHER);
      SetFixtureBounds(s->child, {160, 30, 80, 40});
      LayoutController::Get(s->window).ProcessLayouts();
      r.Rect("cleared.target", LayoutValidation::Bounds(s->child), {160, 30, 80, 40});
      r.Equal("cancel.no-finish", s->finishes[2], 0);
      s->transition.SetChangeTiming(Timing(0, 10));
      SetFixtureBounds(s->child, {200, 30, 80, 40});
      LayoutController::Get(s->window).ProcessLayouts();
      r.Near("zero-duration.ignore-delay", LayoutValidation::Bounds(s->child).x, 200);
    }}}});
    return cases;
  }
};
REGISTER_MANUAL_TEST(TcLr37)
