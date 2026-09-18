/* Copyright (c) 2026 Samsung Electronics Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 */
#include "layout-validation-workloads.h"
#include <chrono>

using namespace LayoutValidation;
namespace
{
enum class Phase
{
  CONSTRUCT,
  FIRST_PASS,
  WARM_PASS,
  IDLE_PASS
};
const char* PhaseName(Phase phase)
{
  switch(phase)
  {
    case Phase::CONSTRUCT:
      return "construction";
    case Phase::FIRST_PASS:
      return "first_pass";
    case Phase::WARM_PASS:
      return "warm_pass";
    case Phase::IDLE_PASS:
      return "idle_pass";
  }
  return "invalid";
}
constexpr uint32_t WARM_OPERATIONS = 32;
constexpr uint32_t IDLE_OPERATIONS = 4096;
constexpr uint32_t SAMPLES         = 41;
constexpr uint32_t WARMUP          = 10;

// Immediate observation right after the timed region: requested inputs for construction,
// otherwise the controller-produced measured slots and event-side arranged geometry.
ImmediateWorkResult ObserveImmediate(const Workload& w, Phase phase)
{
  ImmediateWorkResult result;
  if(phase == Phase::CONSTRUCT)
  {
    result.Add(w.root.GetRequestedWidth(), w.width);
    result.Add(w.root.GetRequestedHeight(), w.height);
    result.Add(static_cast<float>(w.root.GetChildViewCount()), static_cast<float>(w.leaves.size()));
    for(const auto& leaf : w.leaves)
    {
      result.Add(leaf.GetRequestedWidth(), 8);
      result.Add(leaf.GetRequestedHeight(), 16);
    }
  }
  else
  {
    result.Add(w.root.GetMeasuredSize().width, w.width);
    result.Add(w.root.GetMeasuredSize().height, w.height);
    result.Add(LayoutValidation::Bounds(w.root), {0, 0, w.width, w.height});
    ObserveMeasuredSlots(result, w);
    ObserveArrangedGeometry(result, w);
  }
  return result;
}
uint64_t ImmediateValues(Phase phase, uint32_t count)
{
  return phase == Phase::CONSTRUCT ? 3 + 2ull * count : 12 + 6ull * count;
}
// Performs the timed operations of `phase` on a mounted workload and returns the elapsed time.
double TimePhase(Run& r, Workload& w, WorkloadStage& host, Phase phase)
{
  const auto start = std::chrono::steady_clock::now();
  switch(phase)
  {
    case Phase::CONSTRUCT:
      break;
    case Phase::FIRST_PASS:
      ProcessNow(r);
      break;
    case Phase::WARM_PASS:
      for(uint32_t i = 0; i < WARM_OPERATIONS; ++i)
      {
        host.Nudge();
        ProcessNow(r);
      }
      break;
    case Phase::IDLE_PASS:
      for(uint32_t i = 0; i < IDLE_OPERATIONS; ++i) ProcessNow(r);
      break;
  }
  return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - start).count();
}

void CheckWork(Run& r, WorkloadKind kind, uint32_t count, Phase phase)
{
  using K = WorkDiagnostic::StorageSite;
  auto w  = FlatWorkload(kind, count);
  // Geometry writes required by the first pass are counted from the fresh actor state.
  const uint64_t firstWrites = RequiredGeometryWrites(w);
  auto           host        = MountWorkload(r, w, true);
  if(phase != Phase::FIRST_PASS) ProcessNow(r); // settle outside the capture
  WorkBudget budget;
  if(phase == Phase::FIRST_PASS)
  {
    budget.measureEnter = budget.measureProducer = budget.measurePublish = count + 1;
    budget.ancestorVisits                                                = count;
    budget.arrangeEnter = budget.arrangeProducer = budget.arrangePublish = count + 1;
    if(kind == WorkloadKind::GRID)
    {
      budget.measureEnter += count;
      budget.measureHit = count;
    }
    budget.writes    = firstWrites;
    budget.rootDrain = 1;
    MeasureStorage(budget, kind, count);
    ArrangeStorage(budget, kind, count);
  }
  else if(phase == Phase::WARM_PASS)
  {
    budget.measureEnter = budget.measureHit = WARM_OPERATIONS;
    budget.arrangeEnter = budget.arrangeHit = WARM_OPERATIONS;
    budget.replay                           = WARM_OPERATIONS * (count + 1ull);
    budget.rootDrain                        = WARM_OPERATIONS;
    budget.Storage(K::VIEW_REPLAY_CHILDREN, WARM_OPERATIONS, count, sizeof(View));
  }
  WorkCapture capture;
  capture.Begin(w, 5900 + static_cast<uint64_t>(phase));
  TimePhase(r, w, host, phase);
  capture.End();
  const auto immediate = ObserveImmediate(w, phase);
  immediate.Check(r, "immediate", ImmediateValues(phase, count));
  capture.Check(r, budget);
  VerifyWorkload(r, w, "geometry");
  r.RequireExternalVerification("LR59 also requires the matching Release reference/candidate timing experiment from the timing steps");
  VerifyFinalGeometry(r, host, w, "rendered", true);
}

uint32_t Operations(Phase phase)
{
  return phase == Phase::WARM_PASS ? WARM_OPERATIONS : phase == Phase::IDLE_PASS ? IDLE_OPERATIONS : 1u;
}
void Benchmark(Run& r, WorkloadKind kind, uint32_t count, Phase phase)
{
#if defined(DEBUG_ENABLED)
  r.Fail("profile.release", "timing requires a Release application and library");
#else
  r.RequireExternalVerification("compare raw samples against a pinned reference using layout-validation-tools.py and attach the work-budget step results");
  const std::string metric = std::string("LR59.") + KindName(kind) + ".n" + std::to_string(count) + "." + PhaseName(phase);
  WorkloadStage     host;
  Workload          w;
  for(uint32_t sample = 0; sample < SAMPLES; ++sample)
  {
    double elapsed = 0;
    if(phase == Phase::CONSTRUCT)
    {
      const auto start = std::chrono::steady_clock::now();
      w                = FlatWorkload(kind, count);
      elapsed          = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - start).count();
    }
    else
    {
      w = FlatWorkload(kind, count);
    }
    if(!host.stage)
      host = MountWorkload(r, w, true);
    else
      ReplaceWorkload(host, w);
    if(phase != Phase::FIRST_PASS) ProcessNow(r); // settle outside the timed interval
    if(phase != Phase::CONSTRUCT) elapsed = TimePhase(r, w, host, phase);
    const auto immediate = ObserveImmediate(w, phase);
    if(sample >= WARMUP)
    {
      immediate.Check(r, Id("sample.immediate", sample - WARMUP).c_str(), ImmediateValues(phase, count));
      r.Truth(Id("sample.timer_positive", sample - WARMUP).c_str(), elapsed > 0 && std::isfinite(elapsed));
      VerifyWorkload(r, w, Id("sample.geometry", sample - WARMUP).c_str());
      r.Sample(metric.c_str(), sample - WARMUP, elapsed, Operations(phase));
    }
  }
  VerifyFinalGeometry(r, host, w, "rendered", true);
#endif
}

class TcLr59 : public Case
{
public:
  TcLr59()
  : Case("LR59", "Compute performance")
  {
  }
  std::vector<Scenario> BuildScenarios() override
  {
    std::vector<Scenario> result;
    for(auto kind : {WorkloadKind::STACK, WorkloadKind::FLEX, WorkloadKind::GRID, WorkloadKind::ABSOLUTE})
      for(uint32_t count : {16u, 128u, 512u})
      {
        Scenario s{std::string(KindName(kind)) + "-n" + std::to_string(count), "Controller-driven passes on a rendered workload: immediate results, work budgets and matched Release timing", {}};
        // Timing first, then the work budgets: the timing steps must run on a freshly
        // constructed workload, before any capture has been opened in this scenario.
        for(auto phase : {Phase::CONSTRUCT, Phase::FIRST_PASS, Phase::WARM_PASS, Phase::IDLE_PASS})
          s.steps.push_back({PhaseName(phase), 220u + 4u * count, [kind, count, phase](Run& r) { Benchmark(r, kind, count, phase); }});
        for(auto phase : {Phase::FIRST_PASS, Phase::WARM_PASS, Phase::IDLE_PASS})
          s.steps.push_back({std::string("work-") + PhaseName(phase), WORK_BUDGET_CHECKS + 9 + 4u * count, [kind, count, phase](Run& r) { CheckWork(r, kind, count, phase); }});
        result.push_back(std::move(s));
      }
    return result;
  }
};
} //namespace
REGISTER_MANUAL_TEST(TcLr59)
