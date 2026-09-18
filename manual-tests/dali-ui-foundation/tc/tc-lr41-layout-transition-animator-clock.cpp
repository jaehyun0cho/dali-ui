/* Copyright (c) 2026 Samsung Electronics Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 */
#include <dali/public-api/animation/spring-data.h>
#include "layout-validation-transition-fixtures.h"
using namespace LayoutValidation;
using namespace LayoutValidation::TransitionFixtures;
namespace
{
float CustomCurve(float t)
{
  return 2.0f - .5f * t;
}
struct Curve
{
  const char*          name;
  AlphaFunction        alpha;
  std::array<float, 5> expected;
};
} //namespace
class TcLr41 : public Case
{
public:
  TcLr41()
  : Case("LR41", "Transition animator clock and easing")
  {
  }
  std::vector<Scenario> BuildScenarios() override
  {
    std::vector<Scenario> cases;
    const float              q = std::sqrt(.5f);
    const std::vector<Curve> curves{
      {"default", AlphaFunction(AlphaFunction::DEFAULT), {0, .25, .5, .75, 1}},
      {"linear", AlphaFunction(AlphaFunction::LINEAR), {0, .25, .5, .75, 1}},
      {"square-in", AlphaFunction(AlphaFunction::EASE_IN_SQUARE), {0, .0625, .25, .5625, 1}},
      {"square-out", AlphaFunction(AlphaFunction::EASE_OUT_SQUARE), {0, .4375, .75, .9375, 1}},
      {"cubic-in", AlphaFunction(AlphaFunction::EASE_IN), {0, .015625, .125, .421875, 1}},
      {"cubic-out", AlphaFunction(AlphaFunction::EASE_OUT), {0, .578125, .875, .984375, 1}},
      {"cubic-inout", AlphaFunction(AlphaFunction::EASE_IN_OUT), {0, .0625, .5, .9375, 1}},
      {"sine-in", AlphaFunction(AlphaFunction::EASE_IN_SINE), {0, 1 - std::cos(float(M_PI) / 8), 1 - q, 1 - std::cos(3 * float(M_PI) / 8), 1}},
      {"sine-out", AlphaFunction(AlphaFunction::EASE_OUT_SINE), {0, std::sin(float(M_PI) / 8), q, std::sin(3 * float(M_PI) / 8), 1}},
      {"sine-inout", AlphaFunction(AlphaFunction::EASE_IN_OUT_SINE), {0, (1 - q) / 2, .5, (1 + q) / 2, 1}},
      {"bounce-fallback", AlphaFunction(AlphaFunction::BOUNCE), {0, .25, .5, .75, 1}},
      {"sin-fallback", AlphaFunction(AlphaFunction::SIN), {0, .25, .5, .75, 1}},
      {"back-fallback", AlphaFunction(AlphaFunction::EASE_OUT_BACK), {0, .25, .5, .75, 1}},
      {"bezier-fallback", AlphaFunction(Vector2(.1f, .9f), Vector2(.8f, .2f)), {0, .25, .5, .75, 1}},
      {"spring-fallback", AlphaFunction(AlphaFunction::GENTLE), {0, .25, .5, .75, 1}},
      {"custom-spring-fallback", AlphaFunction(SpringData(100.f, 10.f, 1.f)), {0, .25, .5, .75, 1}},
      {"custom-nonterminal", AlphaFunction(&CustomCurve), {2, 1.875, 1.75, 1.625, 1.5}}};
    for(const auto& curve : curves)
    {
      cases.push_back({"TR07.curve-" + std::string(curve.name), "Actual TickAnimators path, independent quarter-progress table", {{"mount", 4, [](Run& r)
      { Mount(r, NewState(r)); }},
                                                                                                                                  {"tick-five-samples", 68, [curve](Run& r)
      {
        auto s = r.State<State>();
        Watch(s);
        Diagnostics::SetManualAnimatorTicks(s->window, true);
        s->Bind(LayoutTransition::New());
        auto timing  = AnimatorTiming();
        timing.alpha = curve.alpha;
        s->transition.SetChangeAnimator(LayoutAnimatorCallback::New(s.get(), &State::OnAnimator), timing);
        s->root.SetLayoutTransition(s->transition);
        SetFixtureBounds(s->child, {140, 50, 120, 60});
        LayoutController::Get(s->window).ProcessLayouts();
        for(unsigned i = 0; i < 5; ++i)
        {
          const auto prefix = Id("tick", i);
          Tick(r, s, .1f, (prefix + ".real_tick").c_str());
          const auto& observation = s->observations[i];
          r.Near((prefix + ".raw").c_str(), observation.raw, i * .25, .00001);
          r.Near((prefix + ".alpha").c_str(), observation.progress, curve.expected[i], .00001);
          CheckObservation(r, observation, LayoutTransitionSlot::CHANGE, LayoutChangeCause::OTHER,
                           {40, 30, 80, 40}, {140, 50, 120, 60}, prefix.c_str());
        }
        r.Equal("tick.lifecycle.start", s->starts[2], 1);
        r.Equal("tick.lifecycle.finish", s->finishes[2], 1);
        r.Truth("tick.capture.complete", !s->observationOverflow && s->observationCount == 5);
      }}}});
    }
    cases.push_back({"TR07.delay-cap-restart", "Delay, freshly-created zero delta, cap, drained restart and real timer restoration", {{"mount", 4, [](Run& r)
    { Mount(r, NewState(r)); }},
                                                                                                                                      {"delay-and-cap", 12, [](Run& r)
    {
      auto s = r.State<State>();
      Diagnostics::SetManualAnimatorTicks(s->window, true);
      CaptureLifecycle(s);
      s->transition.SetChangeAnimator(LayoutAnimatorCallback::New(s.get(), &State::OnAnimator), AnimatorTiming(.2f, .2f));
      SetFixtureBounds(s->child, {140, 30, 80, 40});
      LayoutController::Get(s->window).ProcessLayouts();
      const float expected[]  = {0, 0, 0, .5f, 1};
      unsigned    sampleIndex = 0;
      for(float value : expected)
      {
        const auto prefix = Id("cap.tick", sampleIndex++);
        Tick(r, s, 1.0f, (prefix + ".real_tick").c_str());
        r.Near((prefix + ".raw").c_str(), LastObservation(*s).raw, value, .00001);
      }
      r.Equal("cap.finish.once", s->finishes[2], 1);
      r.Truth("cap.drained", !Diagnostics::GetTransitionSnapshot(s->child).animatorActive);
    }},
                                                                                                                                      {"restore-natural-timer", 4, [](Run& r)
    {
      auto s = r.State<State>();
      s->ResetObservations();
      s->transition.SetChangeAnimator(LayoutAnimatorCallback::New(s.get(), &State::OnAnimator), AnimatorTiming(.15f));
      SetFixtureBounds(s->child, {40, 30, 80, 40});
      LayoutController::Get(s->window).ProcessLayouts();
      Tick(r, s, .1f);
      r.Near("restart.first.raw", s->observations[0].raw, 0, .00001);
      Diagnostics::SetManualAnimatorTicks(s->window, false);
      r.Delay(650, [s](Run& v)
      {
        v.Equal("restart.natural.finish", s->finishes[2], 1);
        v.Truth("restart.natural.raw-one", s->observationCount > 1 && s->observations[s->observationCount - 1].raw == 1.f);
      });
    }}}});
    for(float duration : {0.f, -.1f})
      cases.push_back({"TR07.nonpositive-" + std::to_string(duration), "Non-positive duration ignores delay and completes in its first tick", {{"mount", 4, [](Run& r)
      { Mount(r, NewState(r)); }},
                                                                                                                                               {"instant-tick", 5, [duration](Run& r)
      {
        auto s = r.State<State>();
        Diagnostics::SetManualAnimatorTicks(s->window, true);
        CaptureLifecycle(s);
        s->transition.SetChangeAnimator(LayoutAnimatorCallback::New(s.get(), &State::OnAnimator), AnimatorTiming(duration, 10));
        SetFixtureBounds(s->child, {140, 30, 80, 40});
        LayoutController::Get(s->window).ProcessLayouts();
        Tick(r, s, 0);
        r.Equal("instant.callbacks", s->observationCount, 1);
        r.Near("instant.raw", s->observations[0].raw, 1);
        r.Equal("instant.start", s->starts[2], 1);
        r.Equal("instant.finish", s->finishes[2], 1);
      }}}});
    return cases;
  }
};
REGISTER_MANUAL_TEST(TcLr41)
