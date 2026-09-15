/* Copyright (c) 2026 Samsung Electronics Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */
#include "layout-validation-workloads.h"
using namespace LayoutValidation;
namespace
{
enum class Phase
{
  CONSTRUCT,
  FIRST_MEASURE,
  FIRST_ARRANGE,
  WARM_MEASURE,
  WARM_ARRANGE
};
const char* PhaseName(Phase phase)
{
  switch(phase)
  {
    case Phase::CONSTRUCT:
      return "construction";
    case Phase::FIRST_MEASURE:
      return "first_measure";
    case Phase::FIRST_ARRANGE:
      return "first_arrange";
    case Phase::WARM_MEASURE:
      return "warm_measure";
    case Phase::WARM_ARRANGE:
      return "warm_arrange";
  }
  return "invalid";
}

ImmediateWorkResult ObserveImmediate(const Workload& w, Phase phase, const MeasuredSize* measures, const LayoutRect* arrangements, uint32_t operations)
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
  else if(phase == Phase::FIRST_MEASURE || phase == Phase::WARM_MEASURE)
  {
    for(uint32_t i = 0; i < operations; ++i)
    {
      result.Add(measures[i].width, w.width);
      result.Add(measures[i].height, w.height);
    }
    ObserveMeasuredSlots(result, w);
  }
  else
  {
    for(uint32_t i = 0; i < operations; ++i) result.Add(arrangements[i], {0, 0, w.width, w.height});
    ObserveArrangedGeometry(result, w);
  }
  return result;
}

uint64_t ImmediateValues(Phase phase, uint32_t count, uint32_t operations)
{
  if(phase == Phase::CONSTRUCT) return 3 + 2ull * count;
  if(phase == Phase::FIRST_MEASURE || phase == Phase::WARM_MEASURE) return 2ull * operations + 2 + 2ull * count;
  return 4ull * operations + 4 + 4ull * count;
}

#if defined(DALI_UI_LAYOUT_TEST_DIAGNOSTICS)
void CheckWork(Run& r, WorkloadKind kind, uint32_t count, Phase phase)
{
  using K = WorkDiagnostic::StorageSite;
  auto w  = FlatWorkload(kind, count);
  if(phase == Phase::FIRST_ARRANGE) w.root.Measure(w.width, w.height);
  if(phase == Phase::WARM_MEASURE || phase == Phase::WARM_ARRANGE) Settle(w);
  const uint32_t operations = phase == Phase::WARM_MEASURE || phase == Phase::WARM_ARRANGE ? 8u : 1u;
  WorkBudget     budget;
  if(phase == Phase::FIRST_MEASURE)
  {
    budget.measureEnter = budget.measureProducer = budget.measurePublish = count + 1;
    budget.ancestorVisits                                                = count;
    MeasureStorage(budget, kind, count);
  }
  else if(phase == Phase::FIRST_ARRANGE)
  {
    budget.arrangeEnter = budget.arrangeProducer = budget.arrangePublish = count + 1;
    if(kind == WorkloadKind::GRID) budget.measureEnter = budget.measureHit = count;
    budget.writes = RequiredGeometryWrites(w);
    ArrangeStorage(budget, kind, count);
  }
  else if(phase == Phase::WARM_MEASURE)
  {
    budget.measureEnter = budget.measureHit = operations;
  }
  else
  {
    budget.arrangeEnter = budget.arrangeHit = operations;
    budget.replay                           = operations * (count + 1ull);
    budget.Storage(K::VIEW_REPLAY_CHILDREN, operations, count, sizeof(View));
  }
  std::array<MeasuredSize, 8> measures{};
  std::array<LayoutRect, 8>   arrangements{};
  WorkCapture                 capture;
  capture.Begin(w, 5900 + static_cast<uint64_t>(phase));
  for(uint32_t i = 0; i < operations; ++i)
  {
    if(phase == Phase::FIRST_MEASURE || phase == Phase::WARM_MEASURE)
      measures[i] = w.root.Measure(w.width, w.height);
    else
      arrangements[i] = w.root.Arrange({0, 0, w.width, w.height});
  }
  capture.End();
  // Capture the first operation's result before any subsequent layout can repair it.
  const auto immediate = ObserveImmediate(w, phase, measures.data(), arrangements.data(), operations);
  immediate.Check(r, "immediate", ImmediateValues(phase, count, operations));
  capture.Check(r, budget);
  if(phase == Phase::FIRST_MEASURE || phase == Phase::WARM_MEASURE) Settle(w);
  VerifyWorkload(r, w, "geometry", true);
  r.RequireExternalVerification("LR59 also requires the matching Release OFF reference/candidate timing experiment");
}
#else
void Benchmark(Run& r, WorkloadKind kind, uint32_t count, Phase phase)
{
#if defined(DEBUG_ENABLED)
  r.Fail("profile.release", "timing requires uninstrumented Release application and library");
#else
  r.RequireExternalVerification("compare raw samples against a pinned reference using layout-validation-tools.py and attach ON work budgets");
  const std::string metric = std::string("LR59.") + KindName(kind) + ".n" + std::to_string(count) + "." + PhaseName(phase);
  for(uint32_t sample = 0; sample < 41; ++sample)
  {
    Workload w;
    if(phase != Phase::CONSTRUCT) w = FlatWorkload(kind, count);
    if(phase == Phase::FIRST_ARRANGE) w.root.Measure(w.width, w.height);
    if(phase == Phase::WARM_MEASURE || phase == Phase::WARM_ARRANGE) Settle(w);
    const uint32_t operations = phase == Phase::WARM_MEASURE ? 4096u : phase == Phase::WARM_ARRANGE ? 32u
                                                                                                    : 1u;
    // Returning data is stored in fixed buffers; allocation, checks and formatting are outside the interval.
    std::array<MeasuredSize, 4096> measures{};
    std::array<LayoutRect, 32>     arrangements{};
    const auto                     start = std::chrono::steady_clock::now();
    for(uint32_t i = 0; i < operations; ++i)
    {
      switch(phase)
      {
        case Phase::CONSTRUCT:
          w = FlatWorkload(kind, count);
          break;
        case Phase::FIRST_MEASURE:
        case Phase::WARM_MEASURE:
          measures[i] = w.root.Measure(w.width, w.height);
          break;
        case Phase::FIRST_ARRANGE:
        case Phase::WARM_ARRANGE:
          arrangements[i] = w.root.Arrange({0, 0, w.width, w.height});
          break;
      }
    }
    const double elapsed   = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - start).count();
    const auto   immediate = ObserveImmediate(w, phase, measures.data(), arrangements.data(), operations);
    if(sample >= 10) immediate.Check(r, Id("sample.immediate", sample - 10).c_str(), ImmediateValues(phase, count, operations));
    // Construction/Measure still need an Arrange to inspect eventual geometry. Their
    // original outputs have already been checked. Arrange phases never run a repair pass.
    if(phase == Phase::CONSTRUCT || phase == Phase::FIRST_MEASURE || phase == Phase::WARM_MEASURE) Settle(w);
    if(sample >= 10)
    {
      r.Truth(Id("sample.timer_positive", sample - 10).c_str(), elapsed > 0 && std::isfinite(elapsed));
      VerifyWorkload(r, w, Id("sample.geometry", sample - 10).c_str(), sample == 40);
      r.Sample(metric.c_str(), sample - 10, elapsed, operations);
    }
  }
#endif
}
#endif

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
        Scenario s{std::string(KindName(kind)) + "-n" + std::to_string(count), "Independent immediate results, work budgets and matched Release timing", {}};
#if defined(DALI_UI_LAYOUT_TEST_DIAGNOSTICS)
        for(auto phase : {Phase::FIRST_MEASURE, Phase::FIRST_ARRANGE, Phase::WARM_MEASURE, Phase::WARM_ARRANGE})
          s.steps.push_back({std::string("work-") + PhaseName(phase), WORK_BUDGET_CHECKS + 6 + 4u * count, [kind, count, phase](Run& r)
          { CheckWork(r, kind, count, phase); }});
#else
        for(auto phase : {Phase::CONSTRUCT, Phase::FIRST_MEASURE, Phase::FIRST_ARRANGE, Phase::WARM_MEASURE, Phase::WARM_ARRANGE})
          s.steps.push_back({PhaseName(phase), 217u + 4u * count, [kind, count, phase](Run& r)
          { Benchmark(r, kind, count, phase); }});
#endif
        result.push_back(std::move(s));
      }
    return result;
  }
};
} //namespace
REGISTER_MANUAL_TEST(TcLr59)
