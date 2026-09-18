/* Copyright (c) 2026 Samsung Electronics Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 */
#include <dali-ui-foundation/public-api/views/view-impl.h>
#include <limits>
#include "layout-validation-transition-fixtures.h"
using namespace LayoutValidation;
using namespace LayoutValidation::TransitionFixtures;
namespace
{
constexpr uint32_t SAMPLE_COUNT       = 31;
constexpr uint32_t UPDATE_OPERATIONS  = 9;
constexpr uint32_t DORMANT_OPERATIONS = 128;
constexpr uint32_t WARMUP_OPERATIONS  = 10;

void PerformanceAnimator(const LayoutAnimatorContext&)
{
}
struct PerformanceState : State
{
  std::vector<View> leaves;
  uint32_t          toggle{0};
  uint32_t          sample{0};
  unsigned          mode{0};
  unsigned          expectedCount{0};
  std::string       metric;
  void              Update()
  {
    const float width = (++toggle & 1u) ? 81.f : 80.f;
    for(auto& leaf : leaves) leaf.SetRequestedWidth(width);
    LayoutController::Get(window).ProcessLayouts();
  }
};

std::size_t GeometryChecks(unsigned count)
{
  return 5u * count + 2u;
}

void CheckAllLeafTargets(Run& r, const PerformanceState& state, const char* prefix, float expectedWidth, bool useSnapshot = false)
{
  char id[128];
  std::snprintf(id, sizeof(id), "%s.handles", prefix);
  r.Equal(id, state.leaves.size(), state.expectedCount);
  std::snprintf(id, sizeof(id), "%s.logical-children", prefix);
  r.Equal(id, state.root.GetChildViewCount(), state.expectedCount);
  for(unsigned i = 0; i < state.expectedCount; ++i)
  {
    const auto leaf = i < state.leaves.size() ? state.leaves[i] : View{};
    std::snprintf(id, sizeof(id), "%s.leaf-%u.identity-order", prefix, i);
    r.Truth(id, leaf && state.root.GetChildViewAt(i) == leaf);
    const float invalid = std::numeric_limits<float>::quiet_NaN();
    LayoutRect  actual(invalid, invalid, invalid, invalid);
    if(leaf)
    {
      // This existing const getter copies the completed logical result. It does
      // not arrange, flush events, or read an intermediate animated Actor value.
      actual = useSnapshot ? r.Snapshot(leaf) : Dali::Ui::GetImpl(leaf).GetArrangedBounds();
    }
    std::snprintf(id, sizeof(id), "%s.leaf-%u.target", prefix, i);
    r.Rect(id, actual, {0, static_cast<float>(i), expectedWidth, 1});
  }
}

void MeasureSample(Run& r, std::shared_ptr<PerformanceState> state)
{
  const auto begin = std::chrono::steady_clock::now();
  for(uint32_t operation = 0; operation < UPDATE_OPERATIONS; ++operation) state->Update();
  const auto end = std::chrono::steady_clock::now();

  // Ten warmups leave width 80. An odd operation count makes every sample's
  // final target differ from the preceding sample, including the first sample.
  const float expectedWidth = (state->sample & 1u) ? 80.f : 81.f;
  char        prefix[96];
  std::snprintf(prefix, sizeof(prefix), "performance.sample-%u", state->sample);
  CheckAllLeafTargets(r, *state, prefix, expectedWidth);
  r.Sample(state->metric.c_str(), state->sample, std::chrono::duration<double, std::nano>(end - begin).count(), UPDATE_OPERATIONS);
  if(++state->sample < SAMPLE_COUNT)
    r.Delay(20, [state](Run& next)
    { MeasureSample(next, state); });
  else
  {
    r.RequireExternalVerification("All per-sample geometry rows and the paired reference/candidate fingerprint analysis are required; raw timing has no PASS verdict.");
  }
}

/// Builds the workload and attaches it. Shared by the timing and the work-budget scenario
/// families so both start from an identical fixture.
void MountFixture(Run& r, unsigned mode, unsigned count)
{
  auto s    = std::make_shared<PerformanceState>();
  s->window = r.GetWindow();
  s->root   = StackLayout::New();
  s->root.SetRequestedWidth(300);
  s->root.SetRequestedHeight(200);
  s->mode          = mode;
  s->expectedCount = count;
  s->metric        = "LR63.mode" + std::to_string(mode) + ".n" + std::to_string(count) + (mode == 1 ? ".dormant.ns" : ".update.ns");
  s->leaves.reserve(count);
  for(unsigned i = 0; i < count; ++i)
  {
    auto leaf = Leaf(80, 1);
    s->root.Add(leaf);
    s->leaves.push_back(leaf);
  }
  s->child = s->leaves.front();
  if(mode != 0)
  {
    s->transition = LayoutTransition::New();
    if(mode == 2) s->transition.ClearChangeTiming();
    if(mode == 3) s->transition.SetChangeTiming(Timing(.4f));
    if(mode == 4) s->transition.SetChangeAnimator(LayoutAnimatorCallback::New(&PerformanceAnimator), AnimatorTiming());
    s->root.SetLayoutTransition(s->transition);
  }
  r.SetState(s);
  r.OnCleanup([s]
  { s->Cleanup(); });
  r.Attach(s->root);
  r.AfterLayout(s->leaves, [s](Run& v)
  { CheckAllLeafTargets(v, *s, "performance.fixture", 80, true); });
}

/// Timing samples. Never shares a scenario with the work-budget step: that step arms manual
/// animator ticks (only restored by State::Cleanup at scenario end) and leaves the fixture
/// width toggled, either of which would invalidate the sample expectations here.
void ReleaseSamples(Run& r, unsigned mode)
{
  auto s = r.State<PerformanceState>();
  if(mode == 1)
  {
    for(unsigned i = 0; i < WARMUP_OPERATIONS; ++i) LayoutController::Get(s->window).ProcessLayouts();
    CheckAllLeafTargets(r, *s, "performance.warmup", 80);
    for(unsigned sample = 0; sample < SAMPLE_COUNT; ++sample)
    {
      auto begin = std::chrono::steady_clock::now();
      for(unsigned i = 0; i < DORMANT_OPERATIONS; ++i) LayoutController::Get(s->window).ProcessLayouts();
      auto end = std::chrono::steady_clock::now();
      char prefix[96];
      std::snprintf(prefix, sizeof(prefix), "performance.sample-%u", sample);
      CheckAllLeafTargets(r, *s, prefix, 80);
      r.Sample(s->metric.c_str(), sample, std::chrono::duration<double, std::nano>(end - begin).count(), DORMANT_OPERATIONS);
    }
    r.RequireExternalVerification("Every sample's geometry and paired reference/candidate fingerprints are required; no local timing PASS is produced.");
  }
  else
  {
    for(unsigned i = 0; i < WARMUP_OPERATIONS; ++i) s->Update();
    CheckAllLeafTargets(r, *s, "performance.warmup", 80);
    MeasureSample(r, s);
  }
}

/// Work budget for one transition-dispatch pass, observed through the diagnostics hooks.
void WorkCount(Run& r, unsigned mode, unsigned count)
{
  auto s = r.State<PerformanceState>();
  Diagnostics::SetManualAnimatorTicks(s->window, true);
  Diagnostics::ClearRegisteredNodes();
  for(unsigned i = 0; i < count; ++i) Diagnostics::RegisterNode(s->leaves[i], i + 1);
  auto         events = std::make_unique<std::array<Diagnostics::Event, 32768>>();
  CaptureScope captureScope(events->data(), events->size(), 6301);
  if(mode == 1)
    LayoutController::Get(s->window).ProcessLayouts();
  else
    s->Update();
  auto     capture = captureScope.Finish();
  uint32_t starts  = 0;
  uint32_t ticks   = 0;
  for(std::size_t i = 0; i < capture.count; ++i)
    if((*events)[i].nodeId)
    {
      if((*events)[i].kind == Diagnostics::EventKind::TRANSITION_BEGIN) ++starts;
      if((*events)[i].kind == Diagnostics::EventKind::TRANSITION_TICK) ++ticks;
    }
  r.Truth("performance.trace.complete", !capture.overflow);
  r.Equal("performance.transition.begin-count", starts, mode >= 3 ? count : 0);
  r.Equal("performance.tick-count", ticks, 0);
  CheckAllLeafTargets(r, *s, "performance.work", mode == 1 ? 80.f : 81.f);
  r.RequireExternalVerification("The Release samples and paired comparison are also required for a timing verdict.");
}
} //namespace
class TcLr63 : public Case
{
public:
  TcLr63()
  : Case("LR63", "Transition work and timing regression")
  {
  }
  std::vector<Scenario> BuildScenarios() override
  {
    std::vector<Scenario> cases;
    for(unsigned mode = 0; mode < 5; ++mode)
      for(unsigned count : {1u, 32u, 128u})
      {
        cases.push_back({"TR15.mode-" + std::to_string(mode) + "-n" + std::to_string(count),
                         "No attachment, dormant, disabled, active Spec, active Animator workload",
                         {{"mount", GeometryChecks(count), [mode, count](Run& r)
                           { MountFixture(r, mode, count); }},
                          {"release-samples", (SAMPLE_COUNT + 1u) * GeometryChecks(count), [mode](Run& r)
                           { ReleaseSamples(r, mode); }}}});
        cases.push_back({"TR15W.mode-" + std::to_string(mode) + "-n" + std::to_string(count),
                         "Transition dispatch work budget on the same workload, in its own scenario so the manual animator ticks it arms cannot leak into a timing sample",
                         {{"mount", GeometryChecks(count), [mode, count](Run& r)
                           { MountFixture(r, mode, count); }},
                          {"work-count", GeometryChecks(count) + 3u, [mode, count](Run& r)
                           { WorkCount(r, mode, count); }}}});
      }
    return cases;
  }
};
REGISTER_MANUAL_TEST(TcLr63)
