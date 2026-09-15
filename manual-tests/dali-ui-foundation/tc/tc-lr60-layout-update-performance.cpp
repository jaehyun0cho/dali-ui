/* Copyright (c) 2026 Samsung Electronics Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */
#include "layout-validation-workloads.h"
using namespace LayoutValidation;
namespace
{
#if !defined(DALI_UI_LAYOUT_TEST_DIAGNOSTICS)
void UpdateBenchmark(Run& r, uint32_t count, bool dense, bool noOp)
{
#if defined(DEBUG_ENABLED)
  r.Fail("profile.release", "timing requires uninstrumented Release application and library");
#else
  auto w = FlatWorkload(WorkloadKind::STACK, count);
  Settle(w);
  r.RequireExternalVerification("paired reference/candidate update timing with geometry equality");
  const std::string metric = std::string("LR60.stack.n") + std::to_string(count) + (noOp ? ".same_value" : dense ? ".dense"
                                                                                                                 : ".sparse");
  for(uint32_t sample = 0; sample < 41; ++sample)
  {
    const float height = noOp ? 16.f : ((sample % 2) ? 15.f : 16.f);
    const auto  start  = std::chrono::steady_clock::now();
    if(dense)
      for(auto& leaf : w.leaves) leaf.SetRequestedHeight(height);
    else
      w.leaves[count / 2].SetRequestedHeight(height);
    const auto   measured = w.root.Measure(w.width, w.height);
    const auto   arranged = w.root.Arrange(LayoutRect(0, 0, w.width, w.height));
    const double elapsed  = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - start).count();
    for(uint32_t i = 0; i < count; ++i) w.expected[i].height = dense || i == count / 2 ? height : 16.f;
    ImmediateWorkResult immediate;
    immediate.Add(measured.width, w.width);
    immediate.Add(measured.height, w.height);
    immediate.Add(arranged, {0, 0, w.width, w.height});
    ObserveMeasuredSlots(immediate, w);
    ObserveArrangedGeometry(immediate, w);
    if(sample >= 10)
    {
      immediate.Check(r, Id("sample.immediate", sample - 10).c_str(), 12ull + 6ull * count);
      r.Truth(Id("sample.timer_positive", sample - 10).c_str(), elapsed > 0 && std::isfinite(elapsed));
      VerifyWorkload(r, w, Id("sample.geometry", sample - 10).c_str(), sample == 40);
      r.Sample(metric.c_str(), sample - 10, elapsed, 1);
    }
  }
#endif
}

#else
void UpdateWork(Run& r, uint32_t count, bool dense, bool noOp)
{
  auto w = FlatWorkload(WorkloadKind::STACK, count);
  Settle(w);
  const uint32_t changed = noOp ? 0u : dense ? count
                                             : 1u;
  const float    height  = noOp ? 16.f : 15.f;
  for(uint32_t i = 0; i < count; ++i) w.expected[i].height = dense || i == count / 2 ? height : 16.f;
  WorkBudget budget;
  if(changed)
  {
    budget.measureEnter    = count + 1;
    budget.measureProducer = budget.measurePublish = changed + 1;
    budget.measureHit                              = count - changed;
    budget.arrangeEnter                            = count + 1;
    budget.arrangeProducer = budget.arrangePublish = changed + 1;
    budget.arrangeHit = budget.replay = count - changed;
    budget.writes                     = changed;
    budget.invalidateMeasure          = 2ull * changed;
    budget.ancestorVisits             = 2ull * changed + 1;
    MeasureStorage(budget, WorkloadKind::STACK, count);
    ArrangeStorage(budget, WorkloadKind::STACK, count);
  }
  else
  {
    budget.measureEnter = budget.measureHit = 1;
    budget.arrangeEnter = budget.arrangeHit = 1;
    budget.replay                           = count + 1;
    budget.Storage(WorkDiagnostic::StorageSite::VIEW_REPLAY_CHILDREN, 1, count, sizeof(View));
  }
  WorkCapture capture;
  capture.Begin(w, noOp ? 6003 : dense ? 6002
                                       : 6001);
  if(dense)
    for(auto& leaf : w.leaves) leaf.SetRequestedHeight(height);
  else
    w.leaves[count / 2].SetRequestedHeight(height);
  const auto measured = w.root.Measure(w.width, w.height);
  const auto arranged = w.root.Arrange({0, 0, w.width, w.height});
  capture.End();
  capture.Check(r, budget);
  r.Size("output.measured", measured, {w.width, w.height});
  r.Rect("output.arranged", arranged, {0, 0, w.width, w.height});
  VerifyWorkload(r, w, "geometry", true);
  r.RequireExternalVerification("LR60 also requires the matching Release OFF reference/candidate timing experiment");
}
#endif

class TcLr60 : public Case
{
public:
  TcLr60()
  : Case("LR60", "Update performance")
  {
  }
  std::vector<Scenario> BuildScenarios() override
  {
    std::vector<Scenario> result;
    for(uint32_t count : {16u, 128u, 512u})
    {
      Scenario s{"n" + std::to_string(count), "Sparse, dense and same-value mutation", {}};
#if defined(DALI_UI_LAYOUT_TEST_DIAGNOSTICS)
      s.steps.push_back({"work-sparse", WORK_BUDGET_CHECKS + 9 + 4u * count, [count](Run& r)
      { UpdateWork(r, count, false, false); }});
      s.steps.push_back({"work-dense", WORK_BUDGET_CHECKS + 9 + 4u * count, [count](Run& r)
      { UpdateWork(r, count, true, false); }});
      s.steps.push_back({"work-same-value", WORK_BUDGET_CHECKS + 9 + 4u * count, [count](Run& r)
      { UpdateWork(r, count, true, true); }});
#else
      s.steps.push_back({"sparse", 217u + 4u * count, [count](Run& r)
      { UpdateBenchmark(r, count, false, false); }});
      s.steps.push_back({"dense", 217u + 4u * count, [count](Run& r)
      { UpdateBenchmark(r, count, true, false); }});
      s.steps.push_back({"same-value", 217u + 4u * count, [count](Run& r)
      { UpdateBenchmark(r, count, true, true); }});
#endif
      result.push_back(std::move(s));
    }
    return result;
  }
};
} //namespace
REGISTER_MANUAL_TEST(TcLr60)
