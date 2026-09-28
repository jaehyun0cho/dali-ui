/* Copyright (c) 2026 Samsung Electronics Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 */
#include "layout-validation-transition-fixtures.h"
using namespace LayoutValidation;
using namespace LayoutValidation::TransitionFixtures;

namespace
{
// Seek() with the spec animation kept PLAYING. Seek() Pause()s it, and dali-core then writes the
// paused current values into the child's event-side geometry (FORCE_CURRENT_VALUE); no
// application can do that to a framework transition, and a same-bounds layout pass reconciles
// such a write back to the arranged target. The dali-core documentation only says that a speed
// factor in [0,1] slows an animation down and gives 0 no behaviour of its own; with the current
// implementation, which scales each frame's elapsed time by the factor, 0 stops the progress while
// the state stays PLAYING and the event side keeps the target Play() gave it -- what an in-flight
// transition looks like to layout between two frames. ancestor-replay.progress (still 0 after the
// replay and a fresh frame) and ancestor-replay.before.state check that this still holds. The
// animation's own factor is kept in restoreSpeed on its first hold so the natural finish can give it back.
void HoldSeek(Run& run, std::shared_ptr<State> state, float progress, const LayoutRect& expected, float& restoreSpeed)
{
  state->animation = Diagnostics::GetSpecAnimation(state->child);
  run.Truth("spec.active_handle", static_cast<bool>(state->animation));
  if(!state->animation)
  {
    run.Finish();
    return;
  }
  const float speed = state->animation.GetSpeedFactor();
  if(speed != 0.0f) restoreSpeed = speed;
  state->animation.SetSpeedFactor(0.0f);
  state->animation.SetCurrentProgress(progress);
  run.AfterFrame([state, progress, expected](Run& result)
  {
    result.Near("spec.progress", state->animation.GetCurrentProgress(), progress, 0.0001);
    result.Rect("spec.rendered", Current(state->child), expected, 0.01);
    result.Finish();
  }, [state, progress]
  { return std::abs(state->animation.GetCurrentProgress() - progress) <= 0.0001f; }, state->child);
}

// holdEnter: the ENTER seeks use HoldSeek (see above). The EXIT seeks always use Seek(): an EXIT
// child is a ghost that layout passes do not arrange, so pausing it cannot meet a reconcile.
Scenario BoundsScenario(const std::string& id, LayoutBoundsEffect effect, LayoutRect endpoint, bool rtl = false, bool independentRoot = true, bool holdEnter = false)
{
  auto enterSpeed = std::make_shared<float>(1.0f);
  const LayoutRect base{rtl ? 180.f : 40.f, 30, 80, 40};
  Scenario         scenario{id, "ENTER endpoint to target; EXIT current bounds to endpoint", {}};
  scenario.steps.push_back({"mount-parent", 4, [rtl, independentRoot](Run& r)
  {
    auto s = NewState(r);
    if(rtl) s->root.SetProperty(Actor::Property::LAYOUT_DIRECTION, LayoutDirection::RIGHT_TO_LEFT);
    s->root.Add(s->sibling);
    // Keep result-label invalidation outside the animated fixture tree.
    // Both roots still use the same real Window/controller.
    if(independentRoot)
    {
      s->root.SetProperty(Actor::Property::SENSITIVE, false);
      r.AttachToWindow(s->root);
    }
    else
      r.Attach(s->root);
    r.AfterLayout({s->sibling}, [s, rtl](Run& v)
    { v.Rect("anchor.target", v.Snapshot(s->sibling), {rtl ? 240.f : 0.f, 0, 60, 20}); });
  }});
  for(int sample = 0; sample < 4; ++sample)
  {
    scenario.steps.push_back({"enter-seek-" + std::to_string(sample), 6, [sample, effect, endpoint, base, holdEnter, enterSpeed](Run& r)
    {
      auto s = r.State<State>();
      if(sample == 0)
      {
        s->Bind(LayoutTransition::New());
        s->transition.SetEnterBoundsEffect(effect).SetExitBoundsEffect(effect).ClearChangeTiming();
        s->root.SetLayoutTransition(s->transition);
        s->root.Add(s->child);
        LayoutController::Get(s->window).ProcessLayouts();
      }
      const float t = sample * .25f;
      LayoutRect  expected{endpoint.x + (base.x - endpoint.x) * t, endpoint.y + (base.y - endpoint.y) * t,
                          endpoint.width + (base.width - endpoint.width) * t, endpoint.height + (base.height - endpoint.height) * t};
      if(holdEnter)
        HoldSeek(r, s, t, expected, *enterSpeed);
      else
        Seek(r, s, t, expected);
    }});
  }
  scenario.steps.push_back({"enter-natural-finish", 6, [base, holdEnter, enterSpeed](Run& r)
  {
    auto s = r.State<State>();
    if(!s->animation)
    {
      r.Fail("animation.missing", "no spec animation handle was captured by Seek");
      return;
    }
    if(holdEnter) s->animation.SetSpeedFactor(*enterSpeed);
    s->animation.SetCurrentProgress(0);
    s->animation.Play();
    r.Delay(850, [s, base](Run& v)
    {
      v.Rect("enter.final", Current(s->child), base, .01);
      v.Equal("enter.start.once", s->starts[0], 1);
      v.Equal("enter.finish.once", s->finishes[0], 1);
    });
  }});
  for(int sample = 0; sample < 4; ++sample)
  {
    scenario.steps.push_back({"exit-seek-" + std::to_string(sample), 8, [sample, endpoint, base](Run& r)
    {
      auto s = r.State<State>();
      if(sample == 0) s->root.Remove(s->child, RemovePolicy::ANIMATE_EXIT);
      r.Equal("exit.logical.count", s->root.GetChildViewCount(), 1);
      r.Truth("exit.actor.parent", s->child.GetParent() == s->root);
      const float t = sample * .25f;
      LayoutRect  expected{base.x + (endpoint.x - base.x) * t, base.y + (endpoint.y - base.y) * t,
                          base.width + (endpoint.width - base.width) * t, base.height + (endpoint.height - base.height) * t};
      Seek(r, s, t, expected);
    }});
  }
  scenario.steps.push_back({"exit-natural-finish", 4, [](Run& r)
  {
    auto s = r.State<State>();
    if(!s->animation)
    {
      r.Fail("animation.missing", "no spec animation handle was captured by Seek");
      return;
    }
    s->animation.SetCurrentProgress(0);
    s->animation.Play();
    r.Delay(850, [s](Run& v)
    {
      v.Truth("exit.unparented", !s->child.GetParent());
      v.Equal("exit.start.once", s->starts[1], 1);
      v.Equal("exit.finish.once", s->finishes[1], 1);
      v.Equal("exit.finish.parent.empty", s->parentAtFinish[1], 0);
    });
  }});
  return scenario;
}
} //namespace
class TcLr39 : public Case
{
public:
  TcLr39()
  : Case("LR39", "Transition bounds units and edges")
  {
  }
  std::vector<Scenario> BuildScenarios() override
  {
    std::vector<Scenario> cases;
    for(unsigned edge = 0; edge < 4; ++edge)
      for(unsigned unit = 0; unit < 3; ++unit)
        for(int sign : {-1, 1})
        {
          const bool  horizontal = edge >= 2;
          const float value      = (unit == 0 ? 17.f : .25f) * sign;
          const float magnitude  = unit == 0 ? 17.f : (unit == 1 ? (horizontal ? 80.f : 40.f) : (horizontal ? 300.f : 200.f)) * .25f;
          const float offset     = magnitude * sign * ((edge == 0 || edge == 2) ? -1.f : 1.f);
          LayoutRect  endpoint{40, 30, 80, 40};
          (horizontal ? endpoint.x : endpoint.y) += offset;
          auto effect = LayoutBoundsEffects::SlideFrom(static_cast<LayoutBoundsEdge>(edge),
                                                       LayoutBoundsLength{value, static_cast<LayoutBoundsUnit>(unit)}, Timing(.4f));
          cases.push_back(BoundsScenario("TR05.slide-e" + std::to_string(edge) + "-u" + std::to_string(unit) + "-s" + std::to_string(sign), effect, endpoint));
        }
    for(unsigned edge = 0; edge < 4; ++edge)
    {
      const LayoutRect endpoint[] = {{40, 30, 80, 0}, {40, 70, 80, 0}, {40, 30, 0, 40}, {120, 30, 0, 40}};
      cases.push_back(BoundsScenario("TR05.expand-e" + std::to_string(edge),
                                     LayoutBoundsEffects::ExpandFrom(static_cast<LayoutBoundsEdge>(edge), Timing()), endpoint[edge]));
    }
    for(float factor : {.5f, 1.f, 1.5f})
    {
      // One axis remains nonidentity so factor == 1 still exercises the timed path.
      auto effect = LayoutBoundsEffect().SetTiming(Timing()).SetSizeFactor(factor, .5f).SetAnchor(.5f, .5f);
      cases.push_back(BoundsScenario("TR05.factor-" + std::to_string(factor), effect,
                                     {40 + (80 - 80 * factor) * .5f, 40, 80 * factor, 20}));
    }
    auto mixed = LayoutBoundsEffect().SetTiming(Timing()).SetSizeFactor(.5f, 1.5f).SetAnchor(1, .5f).SetOffset(LayoutBoundsLength::ParentFraction(.1f), LayoutBoundsLength::SelfFraction(-.5f));
    cases.push_back(BoundsScenario("TR05.mixed-units-anchor", mixed, {110, 0, 40, 60}));
    cases.push_back(BoundsScenario("TR14.physical-left-rtl", LayoutBoundsEffects::SlideFrom(LayoutBoundsEdge::LEFT, Timing()), {100, 30, 80, 40}, true));
    auto replay = BoundsScenario("TR14.ancestor-replay-during-enter",
                                 LayoutBoundsEffects::SlideFrom(LayoutBoundsEdge::TOP, LayoutBoundsLength::Pixel(-17), Timing(.4f)),
                                 {40, 47, 80, 40}, false, false, true);
    replay.steps.insert(replay.steps.begin() + 2, {"replay-ancestor", 14, [](Run& r)
    {
      auto s = r.State<State>();
      Watch(s);
      // The ENTER is in flight the way an application's transition is: PLAYING, with the target
      // (40,30,80,40) on the event side since Play(); only the rendered value is at t=0.
      r.Equal("ancestor-replay.before.state", static_cast<int>(s->animation.GetState()), static_cast<int>(Animation::PLAYING));
      r.Near("ancestor-replay.before.event-y", s->child.GetProperty<float>(Actor::Property::POSITION_Y), 30);
      std::array<Diagnostics::Event, 4096> events{};
      CaptureScope                         captureScope(events.data(), events.size(), 3914);
      r.Host().InvalidateArrange();
      LayoutController::Get(s->window).ProcessLayouts();
      const auto capture       = captureScope.Finish();
      bool       childArranged = false;
      int64_t    childWrites   = 0;
      int64_t    childCancels  = 0;
      for(std::size_t i = 0; i < capture.count; ++i)
      {
        if(events[i].nodeId != 2) continue;
        if(events[i].kind == Diagnostics::EventKind::ARRANGE_ENTER || events[i].kind == Diagnostics::EventKind::REPLAY_VISIT) childArranged = true;
        if(events[i].kind == Diagnostics::EventKind::GEOMETRY_WRITE) ++childWrites;
        if(events[i].kind == Diagnostics::EventKind::TRANSITION_CANCEL) ++childCancels;
      }
      r.Truth("ancestor-replay.capture.complete", !capture.overflow);
      r.Truth("ancestor-replay.child-arranged", childArranged);
      // The same-bounds pass reaches the child but must leave the in-flight ENTER alone: no
      // geometry write, no cancellation, the same Animation still PLAYING, the target unchanged.
      r.Equal("ancestor-replay.child-geometry-writes", childWrites, 0);
      r.Equal("ancestor-replay.transition-cancels", childCancels, 0);
      r.Truth("ancestor-replay.same-animation", Diagnostics::GetSpecAnimation(s->child) == s->animation);
      r.Equal("ancestor-replay.after.state", static_cast<int>(s->animation.GetState()), static_cast<int>(Animation::PLAYING));
      r.Near("ancestor-replay.after.event-y", s->child.GetProperty<float>(Actor::Property::POSITION_Y), 30);
      r.AfterFrame([s](Run& v)
      {
        v.Near("ancestor-replay.progress", s->animation.GetCurrentProgress(), 0, .0001);
        v.Rect("ancestor-replay.rendered", Current(s->child), {40, 47, 80, 40}, .01);
      });
    }});
    cases.push_back(std::move(replay));
    return cases;
  }
};
REGISTER_MANUAL_TEST(TcLr39)
