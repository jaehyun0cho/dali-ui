/* Copyright (c) 2026 Samsung Electronics Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */
#include "layout-validation-transition-fixtures.h"
using namespace LayoutValidation;
using namespace LayoutValidation::TransitionFixtures;
namespace
{
void MountIndependentFixture(Run& r)
{
  auto s = NewState(r);
  s->root.Add(s->child);
  s->root.SetProperty(Actor::Property::SENSITIVE, false);
  r.Keep(s->root);
  s->window.Add(s->root);
  r.AfterLayout({s->child}, [s](Run& v)
  { v.Rect("mount.target", v.Snapshot(s->child), {40, 30, 80, 40}); });
}
} // namespace

class TcLr42 : public Case
{
public:
  TcLr42()
  : Case("LR42", "Transition interruption and continuity")
  {
  }
  std::vector<Scenario> BuildScenarios() override
  {
    std::vector<Scenario> cases;
    for(unsigned mode = 0; mode < 4; ++mode)
      cases.push_back({"TR08.same-slot-" + std::to_string(mode), "Spec and Animator replacement preserve the independent quarter-point bounds", {{"mount", 4, [](Run& r)
      { MountIndependentFixture(r); }},
                                                                                                                                                 {"first-quarter", 6, [mode](Run& r)
      {
        if(!r.RequireDiagnostics()) return;
#if defined(DALI_UI_LAYOUT_TEST_DIAGNOSTICS)
        auto s = r.State<State>();
        Diagnostics::SetManualAnimatorTicks(s->window, true);
        s->Bind(LayoutTransition::New());
        s->transition.SetChangeTiming(Timing());
        if(mode & 1) s->transition.SetChangeAnimator(LayoutAnimatorCallback::New(s.get(), &State::OnAnimator), AnimatorTiming());
        s->root.SetLayoutTransition(s->transition);
        SetFixtureBounds(s->child, {140, 30, 80, 40});
        LayoutController::Get(s->window).ProcessLayouts();
        if(mode & 1)
        {
          Diagnostics::TickAnimatorsForTesting(s->window, 0);
          Diagnostics::TickAnimatorsForTesting(s->window, .1f);
          const auto snapshot = Diagnostics::GetTransitionSnapshot(s->child);
          r.Truth("first.animator.active", snapshot.animatorActive);
          r.Near("first.raw", s->observations[s->observationCount - 1].raw, .25);
          r.Rect("first.engine.lerp", snapshot.lastLerped, {65, 30, 80, 40});
        }
        else
          Seek(r, s, .25f, {65, 30, 80, 40});
#endif
      }},
                                                                                                                                                 {"replace", 11, [mode](Run& r)
      {
        if(!r.RequireDiagnostics()) return;
#if defined(DALI_UI_LAYOUT_TEST_DIAGNOSTICS)
        auto s = r.State<State>();
        if(mode & 2)
          s->transition.SetChangeAnimator(LayoutAnimatorCallback::New(s.get(), &State::OnAnimator), AnimatorTiming());
        else
          s->transition.ClearChangeAnimator();
        SetFixtureBounds(s->child, {240, 30, 80, 40});
        LayoutController::Get(s->window).ProcessLayouts();
        const auto snapshot = Diagnostics::GetTransitionSnapshot(s->child);
        r.Rect("replacement.from-quarter", snapshot.from, {65, 30, 80, 40}, .01);
        r.Rect("replacement.to", snapshot.to, {240, 30, 80, 40});
        r.Equal("replacement.start", s->starts[2], 2);
        r.Equal("cancelled.finish.silent", s->finishes[2], 0);
        r.Truth("replacement.active", snapshot.animatorActive || snapshot.specActive);
        if(!(mode & 2))
        {
          s->animation = Diagnostics::GetSpecAnimation(s->child);
          s->animation.Pause();
        }
#endif
      }},
                                                                                                                                                 {"complete-replacement", 3, [mode](Run& r)
      {
        if(!r.RequireDiagnostics()) return;
#if defined(DALI_UI_LAYOUT_TEST_DIAGNOSTICS)
        auto s = r.State<State>();
        if(mode & 2)
          Diagnostics::SetManualAnimatorTicks(s->window, false);
        else
          s->animation.Play();
        r.Delay(900, [s](Run& v)
        {
          v.Equal("normal.finish.only-new", s->finishes[2], 1);
          auto snapshot = Diagnostics::GetTransitionSnapshot(s->child);
          v.Truth("replacement.drained", !snapshot.specActive && !snapshot.animatorActive);
          v.Truth("replacement.not-exit", static_cast<bool>(s->child.GetParent()));
        });
#endif
      }}}});
    for(bool entering : {false, true})
      cases.push_back({entering ? "TR08.enter-to-exit" : "TR08.change-to-exit", "Cross-slot cancellation preserves visual state and suppresses old completion", {{"mount", 4, [](Run& r)
      { MountIndependentFixture(r); }},
                                                                                                                                                                 {"cross-slot", 7, [entering](Run& r)
      {
        if(!r.RequireDiagnostics()) return;
#if defined(DALI_UI_LAYOUT_TEST_DIAGNOSTICS)
        auto s = r.State<State>();
        Diagnostics::SetManualAnimatorTicks(s->window, true);
        CaptureLifecycle(s);
        if(entering)
        {
          s->root.Remove(s->child);
          s->root.Add(s->child);
        }
        else
          SetFixtureBounds(s->child, {140, 30, 80, 40});
        LayoutController::Get(s->window).ProcessLayouts();
        Diagnostics::TickAnimatorsForTesting(s->window, 0);
        Diagnostics::TickAnimatorsForTesting(s->window, .1f);
        s->root.Remove(s->child, RemovePolicy::ANIMATE_EXIT);
        const auto state = Diagnostics::GetTransitionSnapshot(s->child);
        r.Rect("exit.continuity", state.from, entering ? LayoutRect{40, 30, 80, 40} : LayoutRect{65, 30, 80, 40});
        r.Equal("exit.slot", state.slot, static_cast<int>(LayoutTransitionSlot::EXIT));
        r.Equal("old.finish.silent", s->finishes[entering ? 0 : 2], 0);
        r.Equal("exit.start.once", s->starts[1], 1);
#endif
      }},
                                                                                                                                                                 {"exit-finish", 3, [entering](Run& r)
      {
        if(!r.RequireDiagnostics()) return;
#if defined(DALI_UI_LAYOUT_TEST_DIAGNOSTICS)
        auto s = r.State<State>();
        Diagnostics::SetManualAnimatorTicks(s->window, false);
        r.Delay(900, [s, entering](Run& v)
        {
          v.Equal("old.finish.remains-silent", s->finishes[entering ? 0 : 2], 0);
          v.Equal("exit.finish.once", s->finishes[1], 1);
          v.Truth("exit.detached", !s->child.GetParent());
        });
#endif
      }}}});
    for(bool exiting : {false, true})
      cases.push_back({exiting ? "TR08.spec-enter-to-exit" : "TR08.spec-enter-to-change", "Native Spec successor preserves or settles ENTER visual channels", {
        {"mount", 4, [](Run& r) { MountIndependentFixture(r); }},
        {"enter-quarter", 9, [](Run& r)
        {
          if(!r.RequireDiagnostics()) return;
#if defined(DALI_UI_LAYOUT_TEST_DIAGNOSTICS)
          auto s = r.State<State>();
          s->root.Remove(s->child, RemovePolicy::IMMEDIATE);
          s->child.SetProperty(Actor::Property::OPACITY, 0.f);
          s->Bind(LayoutTransition::New());
          auto enter = ViewAnimationSpec::New();
          enter.Opacity(1, Duration(.4f), AlphaFunction(AlphaFunction::LINEAR));
          auto exit = ViewAnimationSpec::New();
          exit.Opacity(0, Duration(.4f), AlphaFunction(AlphaFunction::LINEAR));
          s->transition.SetEnterVisualSpec(enter).SetEnterBoundsEffect(LayoutBoundsEffect().SetTiming(Timing()).SetSizeFactor(.5f, .5f))
            .SetExitVisualSpec(exit).SetExitBoundsEffect(LayoutBoundsEffect().SetTiming(Timing()).SetOffset(LayoutBoundsLength::Pixel(20), LayoutBoundsLength::Pixel(0))).SetChangeTiming(Timing());
          s->root.SetLayoutTransition(s->transition);
          s->root.Add(s->child);
          LayoutController::Get(s->window).ProcessLayouts();
          s->animation = Diagnostics::GetSpecAnimation(s->child);
          r.Truth("cross.enter.active", static_cast<bool>(s->animation));
          if(!s->animation) return;
          s->animation.Pause(); s->animation.SetCurrentProgress(.25f);
          r.Delay(80, [s](Run& v)
          {
            v.Near("cross.enter.progress", s->animation.GetCurrentProgress(), .25, .0001);
            v.Rect("cross.enter.current", Current(s->child), {55,37.5f,50,25}, .01);
            v.Near("cross.enter.opacity", s->child.GetCurrentProperty<float>(Actor::Property::OPACITY), .25, .01);
            v.Equal("cross.enter.started", s->starts[0], 1);
            v.Equal("cross.enter.not-finished", s->finishes[0], 0);
          });
#endif
        }},
        {"successor-half", 14, [exiting](Run& r)
        {
          if(!r.RequireDiagnostics()) return;
#if defined(DALI_UI_LAYOUT_TEST_DIAGNOSTICS)
          auto s = r.State<State>();
          if(exiting) s->root.Remove(s->child, RemovePolicy::ANIMATE_EXIT);
          else { SetFixtureBounds(s->child, {140,30,80,40}); LayoutController::Get(s->window).ProcessLayouts(); }
          const auto snap = Diagnostics::GetTransitionSnapshot(s->child);
          s->animation = Diagnostics::GetSpecAnimation(s->child);
          r.Truth("cross.successor.active", static_cast<bool>(s->animation));
          r.Rect("cross.successor.from", snap.from, {55,37.5f,50,25}, .01);
          r.Equal("cross.successor.slot", snap.slot, static_cast<int>(exiting ? LayoutTransitionSlot::EXIT : LayoutTransitionSlot::CHANGE));
          r.Equal("cross.old-finish-silent", s->finishes[0], 0);
          r.Equal("cross.new-start-once", s->starts[exiting ? 1 : 2], 1);
          if(!s->animation) return;
          s->animation.Pause(); s->animation.SetCurrentProgress(.5f);
          r.Delay(80, [s, exiting](Run& v)
          {
            v.Near("cross.successor.progress", s->animation.GetCurrentProgress(), .5, .0001);
            v.Rect("cross.successor.current", Current(s->child), exiting ? LayoutRect{65,37.5f,50,25} : LayoutRect{97.5f,33.75f,65,32.5f}, .01);
            v.Near("cross.successor.opacity", s->child.GetCurrentProperty<float>(Actor::Property::OPACITY), exiting ? .125 : 1, .01);
          });
#endif
        }},
        {"successor-finish", 9, [exiting](Run& r)
        {
          if(!r.RequireDiagnostics()) return;
#if defined(DALI_UI_LAYOUT_TEST_DIAGNOSTICS)
          auto s = r.State<State>();
          if(!s->animation) { r.Fail("cross.finish.animation", "Actual successor Animation is missing"); return; }
          s->animation.Play();
          r.Delay(750, [s, exiting](Run& v)
          {
            const auto snap = Diagnostics::GetTransitionSnapshot(s->child);
            v.Equal("cross.finish.old-silent", s->finishes[0], 0);
            v.Equal("cross.finish.new-once", s->finishes[exiting ? 1 : 2], 1);
            v.Truth("cross.finish.drained", !snap.specActive && !snap.animatorActive);
            v.Truth("cross.finish.parent", exiting ? !s->child.GetParent() : s->child.GetParent() == s->root);
            v.Rect("cross.finish.current", Current(s->child), exiting ? LayoutRect{75,37.5f,50,25} : LayoutRect{140,30,80,40}, .01);
            v.Near("cross.finish.opacity", s->child.GetCurrentProperty<float>(Actor::Property::OPACITY), exiting ? 0 : 1, .01);
          });
#endif
        }}
      }});
    return cases;
  }
};
REGISTER_MANUAL_TEST(TcLr42)
