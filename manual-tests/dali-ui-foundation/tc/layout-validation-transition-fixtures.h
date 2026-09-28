/* Copyright (c) 2026 Samsung Electronics Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 */
#pragma once

#include <dali-ui-foundation/integration-api/layout-test-diagnostics.h>
#include <dali/public-api/animation/animation.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdexcept>
#include "layout-validation-support.h"

namespace LayoutValidation
{
namespace TransitionFixtures
{
namespace Diagnostics = Dali::Ui::Integration::LayoutTestDiagnostics;

inline LayoutTransitionTiming Timing(float duration = 0.4f, float delay = 0.0f)
{
  return {Duration(duration), AlphaFunction(AlphaFunction::LINEAR), Duration(delay)};
}

inline LayoutAnimatorTiming AnimatorTiming(float duration = 0.4f, float delay = 0.0f)
{
  return {Duration(duration), AlphaFunction(AlphaFunction::LINEAR), Duration(delay)};
}

inline LayoutRect Current(View view)
{
  return {view.GetCurrentProperty<float>(Actor::Property::POSITION_X),
          view.GetCurrentProperty<float>(Actor::Property::POSITION_Y),
          view.GetCurrentProperty<float>(Actor::Property::SIZE_WIDTH),
          view.GetCurrentProperty<float>(Actor::Property::SIZE_HEIGHT)};
}

struct Observation
{
  uint32_t             viewId{0};
  LayoutTransitionSlot slot{LayoutTransitionSlot::CHANGE};
  LayoutChangeCause    cause{LayoutChangeCause::OTHER};
  float                raw{0};
  float                progress{0};
  LayoutRect           from;
  LayoutRect           to;
};

struct State : ConnectionTracker
{
  View                          root;
  View                          child;
  View                          sibling;
  View                          nested;
  View                          other;
  Window                        window;
  Window                        extraWindow;
  LayoutTransition              transition;
  LayoutTransition              secondTransition;
  Animation                     animation;
  std::array<Observation, 4096> observations{};
  std::size_t                   observationCount{0};
  bool                          observationOverflow{false};
  std::array<uint32_t, 3>       starts{};
  std::array<uint32_t, 3>       finishes{};
  std::array<uint32_t, 3>       parentAtFinish{};
  std::array<uint32_t, 64>      completionOrder{};
  std::size_t                   completionCount{0};
  uint32_t                      windowCompletions{0};
  uint32_t                      secondaryCalls{0};
  uint32_t                      measureCalls{0};
  uint32_t                      arrangeCalls{0};
  MeasuredSize                  measuredConstraint;
  bool                          throwMeasure{false};
  bool                          throwArrange{false};
  bool                          reenter{false};
  bool                          removeOnStart{false};
  bool                          removeSiblingOnStart{false};
  bool                          throwOnStart{false};
  bool                          alterAfterRemove{false};
  uint32_t                      interactionAtRemove{0};
  uint32_t                      interactionAtFinish{0};
  bool                          addOnFinish{false};
  bool                          focusReparent{false};
  bool                          removeControllerOnCompletion{false};
  bool                          completionMutation{false};
  bool                          mutationDone{false};

  uint32_t Id(View view) const
  {
    if(view == child) return 2u;
    if(view == sibling) return 3u;
    if(view == nested) return 4u;
    if(view == other) return 5u;
    if(view == root) return 1u;
    return 0u;
  }

  void OnAnimator(const LayoutAnimatorContext& context)
  {
    if(observationCount == observations.size())
    {
      observationOverflow = true;
      return;
    }
    observations[observationCount++] = {Id(context.view), context.slot, context.changeCause,
                                        context.rawProgress, context.progress, context.fromBounds, context.toBounds};
    // Deliberately do not write geometry: from/to/progress are engine outputs.
  }

  void OnSecondaryAnimator(const LayoutAnimatorContext&)
  {
    ++secondaryCalls;
  }

  void OnStart(View view, LayoutTransitionSlot slot)
  {
    ++starts[static_cast<unsigned>(slot)];
    if(throwOnStart) throw std::runtime_error("LR injected lifecycle failure");
    if(removeSiblingOnStart && view == child && !mutationDone)
    {
      mutationDone = true;
      root.Remove(sibling, RemovePolicy::IMMEDIATE);
    }
    if(removeOnStart && view == child && !mutationDone)
    {
      mutationDone = true;
      root.Remove(child, RemovePolicy::IMMEDIATE);
    }
  }

  void OnFinished(View view, LayoutTransitionSlot slot)
  {
    ++finishes[static_cast<unsigned>(slot)];
    if(slot == LayoutTransitionSlot::EXIT) interactionAtFinish = InteractionBits(view);
    parentAtFinish[static_cast<unsigned>(slot)] = view.GetParent() ? 1u : 0u;
    if(addOnFinish && !mutationDone)
    {
      mutationDone = true;
      root.Add(other);
    }
  }

  static uint32_t InteractionBits(Actor actor)
  {
    return (actor.GetProperty<bool>(Actor::Property::SENSITIVE) ? 1u : 0u) |
           (actor.GetProperty<bool>(Actor::Property::FOCUSABLE) ? 2u : 0u) |
           (actor.GetProperty<bool>(Actor::Property::FOCUS_ON_TOUCH) ? 4u : 0u);
  }

  void OnChildRemoved(Actor, Actor removed)
  {
    if(removed != child) return;
    interactionAtRemove = InteractionBits(removed);
    if(alterAfterRemove)
      for(auto property : {Actor::Property::SENSITIVE, Actor::Property::FOCUSABLE, Actor::Property::FOCUS_ON_TOUCH})
        removed.SetProperty(property, false);
  }

  void OnViewFinished(View view, LayoutRect)
  {
    if(completionCount < completionOrder.size()) completionOrder[completionCount++] = Id(view);
    if(completionMutation && !mutationDone)
    {
      mutationDone = true;
      sibling.SetRequestedHeight(31.0f);
    }
  }

  void OnWindowFinished(Window value)
  {
    ++windowCompletions;
    if(completionCount < completionOrder.size()) completionOrder[completionCount++] = 100u;
    if(removeControllerOnCompletion)
    {
      removeControllerOnCompletion = false;
      LayoutController::Remove(value);
    }
  }

  void OnFocusChanged(View oldView, View)
  {
    if(focusReparent && oldView == child)
    {
      focusReparent = false;
      other.Add(child);
    }
  }

  MeasuredSize Measure(View, float width, float height)
  {
    ++measureCalls;
    measuredConstraint = {width, height};
    if(throwMeasure) throw std::runtime_error("LR injected Measure failure");
    if(reenter)
    {
      reenter = false;
      LayoutController::Get(window).ProcessLayouts();
    }
    return {80.0f, 40.0f};
  }

  LayoutRect Arrange(View, const LayoutRect& bounds)
  {
    ++arrangeCalls;
    if(throwArrange) throw std::runtime_error("LR injected Arrange failure");
    return bounds;
  }

  void Bind(LayoutTransition value)
  {
    transition = value;
    transition.SetOnStart(LayoutLifecycleCallback::New(this, &State::OnStart));
    transition.SetOnFinished(LayoutLifecycleCallback::New(this, &State::OnFinished));
  }

  void ResetObservations()
  {
    observationCount    = 0;
    observationOverflow = false;
    starts              = {};
    finishes            = {};
    parentAtFinish      = {};
  }

  void Cleanup()
  {
    DisconnectAll();
    Diagnostics::SetManualAnimatorTicks(window, false);
    Diagnostics::EndCapture();
    Diagnostics::ClearRegisteredNodes();
    if(animation)
    {
      animation.Stop();
      animation.Reset();
    }
    if(root) root.Unparent();
    if(other) other.Unparent();
    if(extraWindow)
    {
      LayoutController::Remove(extraWindow);
      extraWindow.Hide();
      extraWindow.Reset();
    }
    for(View* member : {&root, &child, &sibling, &nested, &other})
    {
      View& view = *member;
      if(view)
      {
        view.SetLayoutTransition({});
        view.SetSelfLayoutTransition({});
        view.SetMeasureCallback({});
        view.SetArrangeCallback({});
      }
    }
    if(transition) transition.SetOnStart({}).SetOnFinished({}).ClearEnterAnimator().ClearExitAnimator().ClearChangeAnimator();
    if(secondTransition) secondTransition.SetOnStart({}).SetOnFinished({}).ClearEnterAnimator().ClearExitAnimator().ClearChangeAnimator();
  }
};

/// The most recent animator observation, or a NaN-progress sentinel when none was recorded, so
/// a missing callback fails its assertion instead of reading before the buffer.
inline const Observation& LastObservation(const State& state)
{
  static const Observation none = [] {
    Observation sentinel;
    sentinel.raw = sentinel.progress = std::numeric_limits<float>::quiet_NaN();
    return sentinel;
  }();
  return state.observationCount ? state.observations[state.observationCount - 1] : none;
}

inline std::shared_ptr<State> NewState(Run& run, bool stack = false)
{
  auto state    = std::make_shared<State>();
  state->window = run.GetWindow();
  state->root   = stack ? View(StackLayout::New()) : View(AbsoluteLayout::New());
  state->root.SetRequestedWidth(300.0f);
  state->root.SetRequestedHeight(200.0f);
  state->root.SetBackgroundColor(FixtureColor(1));
  state->child   = Leaf(80.0f, 40.0f, "transition.child");
  state->sibling = Leaf(60.0f, 20.0f, "transition.sibling");
  state->other   = AbsoluteLayout::New();
  state->other.SetRequestedWidth(160.0f);
  state->other.SetRequestedHeight(120.0f);
  state->other.SetBackgroundColor(FixtureColor(2));
  if(!stack) state->child.SetLayoutParams(AbsoluteLayoutParams::New().SetBounds({40.0f, 30.0f, 80.0f, 40.0f}));
  run.SetState(state);
  run.OnCleanup([state]
  { state->Cleanup(); });
  return state;
}

inline void SetFixtureBounds(View view, const LayoutRect& rect)
{
  view.SetLayoutParams(AbsoluteLayoutParams::New().SetBounds(rect));
}

inline void Mount(Run& run, std::shared_ptr<State> state, const LayoutRect& expected = {40, 30, 80, 40})
{
  state->root.Add(state->child);
  run.Attach(state->root);
  run.AfterLayout({state->child}, [state, expected](Run& result)
  {
    result.Rect("mount.target", result.Snapshot(state->child), expected);
    result.Finish();
  });
}

inline void CaptureLifecycle(std::shared_ptr<State> state, float duration = 0.4f)
{
  state->Bind(LayoutTransition::New());
  state->transition.SetChangeAnimator(LayoutAnimatorCallback::New(state.get(), &State::OnAnimator), AnimatorTiming(duration));
  state->transition.SetEnterAnimator(LayoutAnimatorCallback::New(state.get(), &State::OnAnimator), AnimatorTiming(duration));
  state->transition.SetExitAnimator(LayoutAnimatorCallback::New(state.get(), &State::OnAnimator), AnimatorTiming(duration));
  state->root.SetLayoutTransition(state->transition);
}

inline void ExpectException(Run& run, const char* id, const char* message, const std::function<void()>& action)
{
  bool        caught = false;
  std::string condition;
  try
  {
    action();
  }
  catch(const DaliException& error)
  {
    caught    = true;
    condition = error.condition ? error.condition : "";
  }
  run.Truth((std::string(id) + ".DaliException").c_str(), caught);
  run.Text((std::string(id) + ".condition").c_str(), condition, message);
}

inline void CheckObservation(Run& run, const Observation& value, LayoutTransitionSlot slot,
                             LayoutChangeCause cause, const LayoutRect& from, const LayoutRect& to, const char* prefix = "")
{
  const std::string key = prefix && *prefix ? std::string(prefix) + "." : std::string{};
  run.Equal((key + "context.slot").c_str(), static_cast<int>(value.slot), static_cast<int>(slot));
  run.Equal((key + "context.cause").c_str(), static_cast<int>(value.cause), static_cast<int>(cause));
  run.Rect((key + "context.from").c_str(), value.from, from);
  run.Rect((key + "context.to").c_str(), value.to, to);
}

class CaptureScope
{
public:
  CaptureScope(Diagnostics::Event* buffer, std::size_t capacity, uint64_t epoch)
  : mActive(Diagnostics::BeginCapture(buffer, capacity, epoch))
  {
  }
  ~CaptureScope()
  {
    if(mActive) Diagnostics::EndCapture();
  }
  Diagnostics::CaptureResult Finish()
  {
    if(!mActive)
    {
      Diagnostics::CaptureResult failed;
      failed.overflow = true;
      return failed;
    }
    mActive = false;
    return Diagnostics::EndCapture();
  }

private:
  bool mActive;
};
inline void Watch(std::shared_ptr<State> state)
{
  // Every registration must succeed: a refused one silently drops that view's node id from
  // every later observation, so the assertions would read 0 instead of failing. The framework
  // turns the exception into an action.exception row of the step that called this.
  const auto require = [](bool registered)
  {
    if(!registered) throw std::runtime_error("RegisterNode refused");
  };
  Diagnostics::ClearRegisteredNodes();
  require(Diagnostics::RegisterNode(state->root, 1));
  require(Diagnostics::RegisterNode(state->child, 2));
  require(Diagnostics::RegisterNode(state->sibling, 3));
  if(state->nested) require(Diagnostics::RegisterNode(state->nested, 4));
  require(Diagnostics::RegisterNode(state->other, 5));
}

inline void Seek(Run& run, std::shared_ptr<State> state, float progress, const LayoutRect& expected)
{
  state->animation = Diagnostics::GetSpecAnimation(state->child);
  run.Truth("spec.active_handle", static_cast<bool>(state->animation));
  if(!state->animation)
  {
    run.Finish();
    return;
  }
  state->animation.Pause();
  state->animation.SetCurrentProgress(progress);
  run.AfterFrame([state, progress, expected](Run& result)
  {
    result.Near("spec.progress", state->animation.GetCurrentProgress(), progress, 0.0001);
    result.Rect("spec.rendered", Current(state->child), expected, 0.01);
    result.Finish();
  }, [state, progress]
  { return std::abs(state->animation.GetCurrentProgress() - progress) <= 0.0001f; }, state->child);
}

inline void Tick(Run& run, std::shared_ptr<State> state, float delta, const char* id = "clock.real_tick")
{
  run.Truth(id, Diagnostics::TickAnimatorsForTesting(state->window, delta));
}
} // namespace TransitionFixtures
} // namespace LayoutValidation
