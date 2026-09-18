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
#include <dali-ui-foundation/public-api/layouts/layout-controller.h>
#include <array>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>
#include "layout-validation-support.h"

namespace LayoutValidation
{
enum class WorkloadKind
{
  STACK,
  FLEX,
  GRID,
  ABSOLUTE
};
inline const char* KindName(WorkloadKind kind)
{
  switch(kind)
  {
    case WorkloadKind::STACK:
      return "stack";
    case WorkloadKind::FLEX:
      return "flex";
    case WorkloadKind::GRID:
      return "grid";
    case WorkloadKind::ABSOLUTE:
      return "absolute";
  }
  return "invalid";
}
struct Workload
{
  View                    root;
  std::vector<View>       leaves;
  std::vector<LayoutRect> expected;
  float                   width{0};
  float                   height{16};
};
inline Workload FlatWorkload(WorkloadKind kind, uint32_t count)
{
  Workload w;
  w.width = count ? count * 10.f - 2.f : 0;
  switch(kind)
  {
    case WorkloadKind::STACK:
    {
      auto root = StackLayout::New(StackOrientation::HORIZONTAL);
      root.SetSpacing(2);
      w.root = root;
      break;
    }
    case WorkloadKind::FLEX:
    {
      auto root = FlexLayout::New();
      root.SetDirection(FlexDirection::ROW);
      root.SetWrap(FlexWrap::NO_WRAP);
      root.SetJustifyContent(FlexJustify::FLEX_START);
      root.SetAlignItems(FlexAlign::FLEX_START);
      w.root = root;
      break;
    }
    case WorkloadKind::GRID:
    {
      auto root = GridLayout::New();
      root.SetColumnSpacing(2);
      root.AddRowDefinition(GridLength::Absolute(16));
      for(uint32_t i = 0; i < count; ++i) root.AddColumnDefinition(GridLength::Absolute(8));
      w.root = root;
      break;
    }
    case WorkloadKind::ABSOLUTE:
      w.root = AbsoluteLayout::New();
      break;
  }
  w.root.SetRequestedWidth(w.width);
  w.root.SetRequestedHeight(w.height);
  w.leaves.reserve(count);
  w.expected.reserve(count);
  for(uint32_t i = 0; i < count; ++i)
  {
    auto leaf = Leaf(8, 16, Id("workload.leaf", i).c_str());
    if(kind == WorkloadKind::FLEX)
    {
      leaf.SetLayoutParams(FlexLayoutParams::New().SetFlexBasis(8).SetFlexGrow(0).SetFlexShrink(0));
      if(i + 1 < count) leaf.SetMargin(Insets(0, 2, 0, 0));
    }
    if(kind == WorkloadKind::GRID) leaf.SetLayoutParams(GridLayoutParams::New().SetColumn(i));
    if(kind == WorkloadKind::ABSOLUTE) leaf.SetLayoutParams(AbsoluteLayoutParams::New().SetBounds(LayoutRect(i * 10.f, 0, 8, 16)));
    w.root.Add(leaf);
    w.leaves.push_back(leaf);
    w.expected.emplace_back(i * 10.f, 0, 8, 16);
  }
  return w;
}
/// On-screen host for a workload: an opaque stage holding the workload root plus a sibling
/// leaf whose width can be nudged to force a controller pass that does not touch the root.
struct WorkloadStage
{
  View  stage, sibling, root;
  float nudge{10};
  void  Nudge()
  {
    sibling.SetRequestedWidth(nudge += 1);
  }
};
inline WorkloadStage MountWorkload(Run& r, const Workload& w, bool isolateRoot = false)
{
  WorkloadStage host;
  host.root    = w.root;
  host.sibling = Leaf(10, 10, "workload.sibling");
  host.sibling.SetRequestedY(w.height + 10);
  const float stageW = std::max(60.0f, std::min(w.width, 460.0f)), stageH = w.height + 30;
  // isolateRoot: attach as an independent Window layout root so a diagnostic work-count capture sees
  // a single drain of the workload (see StageOnWindow). Otherwise mount under the fixture host.
  host.stage = isolateRoot ? r.StageOnWindow(stageW, stageH) : r.Stage(stageW, stageH);
  ColorizeTree(w.root);
  host.stage.Add(w.root);
  host.stage.Add(host.sibling);
  return host;
}
/// Replaces the workload root inside an existing stage (used by per-sample benchmarks).
inline void ReplaceWorkload(WorkloadStage& host, const Workload& w)
{
  if(host.root && host.root.GetParent()) host.root.Unparent();
  host.root = w.root;
  ColorizeTree(w.root);
  host.stage.Add(w.root);
  host.stage.SetRequestedHeight(w.height + 30);
  host.sibling.SetRequestedY(w.height + 10);
}
/// Runs one synchronous LayoutController pass over the window (the controller path used by
/// applications); pending roots are drained and completion events are queued as usual.
inline void ProcessNow(Run& r)
{
  LayoutController::Get(r.GetWindow()).ProcessLayouts();
}
/// Compares the rendered rectangle (last update-thread state, parent relative) of every leaf
/// with the independent expectation. Emits 3 aggregate rows plus, when `rows`, 4 rows per leaf.
inline void VerifyRendered(Run& run, const Workload& w, const char* id, bool rows = false)
{
  uint32_t mismatches   = 0;
  double   maximumError = 0;
  for(std::size_t i = 0; i < w.leaves.size(); ++i)
  {
    const auto  actual   = Run::RenderedBounds(w.leaves[i]);
    const auto  expected = w.expected[i];
    const float a[]      = {actual.x, actual.y, actual.width, actual.height};
    const float e[]      = {expected.x, expected.y, expected.width, expected.height};
    for(unsigned j = 0; j < 4; ++j)
    {
      const double error = std::abs(static_cast<double>(a[j]) - e[j]);
      if(!std::isfinite(a[j]) || error > GEOMETRY_TOLERANCE) ++mismatches;
      if(!std::isfinite(error))
        maximumError = std::numeric_limits<double>::infinity();
      else
        maximumError = std::max(maximumError, error);
    }
    if(rows) run.Rect(Id(id, static_cast<uint32_t>(i)).c_str(), actual, expected);
  }
  run.Equal((std::string(id) + ".visited").c_str(), w.leaves.size(), w.expected.size());
  run.Equal((std::string(id) + ".mismatches").c_str(), mismatches, 0);
  run.Near((std::string(id) + ".maximum_error").c_str(), maximumError, 0);
}
inline void VerifyWorkload(Run& run, const Workload& w, const char* id, bool rows = false)
{
  uint32_t mismatches   = 0;
  double   maximumError = 0;
  for(std::size_t i = 0; i < w.leaves.size(); ++i)
  {
    const auto  actual   = Bounds(w.leaves[i]);
    const auto  expected = w.expected[i];
    const float a[]      = {actual.x, actual.y, actual.width, actual.height};
    const float e[]      = {expected.x, expected.y, expected.width, expected.height};
    for(unsigned j = 0; j < 4; ++j)
    {
      const double error = std::abs(static_cast<double>(a[j]) - e[j]);
      if(!std::isfinite(a[j]) || error > GEOMETRY_TOLERANCE) ++mismatches;
      if(!std::isfinite(error))
        maximumError = std::numeric_limits<double>::infinity();
      else
        maximumError = std::max(maximumError, error);
    }
    if(rows) run.Rect(Id(id, static_cast<uint32_t>(i)).c_str(), actual, expected);
  }
  run.Equal((std::string(id) + ".visited").c_str(), w.leaves.size(), w.expected.size());
  run.Equal((std::string(id) + ".mismatches").c_str(), mismatches, 0);
  run.Near((std::string(id) + ".maximum_error").c_str(), maximumError, 0);
}
/// Checks the final mounted workload after a fresh render task, outside every timed interval.
/// Copy the handles and independent expected values because the caller's workload is local.
inline void VerifyFinalGeometry(Run& r, const WorkloadStage& host, const Workload& w, const char* id, bool rows)
{
  const std::string prefix(id);
  r.AfterFrame([w, prefix, rows](Run& done)
  { VerifyRendered(done, w, prefix.c_str(), rows); }, {}, host.stage);
}

struct ImmediateWorkResult
{
  uint64_t values{0};
  uint64_t mismatches{0};
  double   maximumError{0};
  void     Add(float actual, float expected)
  {
    ++values;
    const double error = std::abs(static_cast<double>(actual) - expected);
    if(!std::isfinite(actual) || error > GEOMETRY_TOLERANCE) ++mismatches;
    maximumError = std::isfinite(error) ? std::max(maximumError, error) : std::numeric_limits<double>::infinity();
  }
  void Add(const LayoutRect& actual, const LayoutRect& expected)
  {
    Add(actual.x, expected.x);
    Add(actual.y, expected.y);
    Add(actual.width, expected.width);
    Add(actual.height, expected.height);
  }
  void Check(Run& run, const char* id, uint64_t requiredValues) const
  {
    run.Equal((std::string(id) + ".values").c_str(), values, requiredValues);
    run.Equal((std::string(id) + ".mismatches").c_str(), mismatches, 0);
    run.Near((std::string(id) + ".maximum_error").c_str(), maximumError, 0);
  }
};

inline void ObserveMeasuredSlots(ImmediateWorkResult& result, const Workload& w)
{
  const auto root = w.root.GetMeasuredSize();
  result.Add(root.width, w.width);
  result.Add(root.height, w.height);
  for(std::size_t i = 0; i < w.leaves.size(); ++i)
  {
    const auto slot = w.leaves[i].GetMeasuredSize();
    result.Add(slot.width, w.expected[i].width);
    result.Add(slot.height, w.expected[i].height);
  }
}

inline void ObserveArrangedGeometry(ImmediateWorkResult& result, const Workload& w)
{
  result.Add(LayoutValidation::Bounds(w.root), {0, 0, w.width, w.height});
  for(std::size_t i = 0; i < w.leaves.size(); ++i)
    result.Add(LayoutValidation::Bounds(w.leaves[i]), w.expected[i]);
}

namespace WorkDiagnostic                 = Dali::Ui::Integration::LayoutTestDiagnostics;
constexpr std::size_t WORK_STORAGE_RULES = 4;
constexpr std::size_t WORK_EVENT_KINDS   = 15;
constexpr std::size_t WORK_BUDGET_CHECKS = 4 + WORK_EVENT_KINDS + 2 + 3 * WORK_STORAGE_RULES;
struct StorageBudget
{
  WorkDiagnostic::StorageSite site{};
  uint64_t                    calls{0};
  uint64_t                    elements{0};
  uint64_t                    bytes{0};
};
struct WorkBudget
{
  uint64_t                                      measureEnter{0}, measureHit{0}, measureProducer{0}, measurePublish{0};
  uint64_t                                      arrangeEnter{0}, arrangeHit{0}, arrangeProducer{0}, arrangePublish{0};
  uint64_t                                      replay{0}, writes{0}, wake{0}, rootDrain{0}, parked{0}, invalidateMeasure{0}, ancestorVisits{0};
  std::array<StorageBudget, WORK_STORAGE_RULES> storage{};
  std::size_t                                   usedStorage{0};
  void                                          Storage(WorkDiagnostic::StorageSite site, uint64_t calls, uint64_t elementsPerCall, std::size_t elementSize)
  {
    if(usedStorage >= storage.size()) throw std::logic_error("work storage budget capacity");
    storage[usedStorage++] = {site, calls, calls * elementsPerCall, calls * elementsPerCall * elementSize};
  }
};

inline void MeasureStorage(WorkBudget& budget, WorkloadKind kind, uint32_t count)
{
  using S = WorkDiagnostic::StorageSite;
  switch(kind)
  {
    case WorkloadKind::STACK:
      budget.Storage(S::STACK_MEASURE_CHILDREN, 1, count, sizeof(View));
      budget.Storage(S::STACK_MEASURE_WORK, 1, count, sizeof(MeasuredSize));
      break;
    case WorkloadKind::FLEX:
      budget.Storage(S::FLEX_MEASURE_CHILDREN, 1, count, sizeof(View));
      break;
    case WorkloadKind::GRID:
      budget.Storage(S::GRID_MEASURE_CHILDREN, 1, count, sizeof(View));
      budget.Storage(S::GRID_MEASURE_ROWS, 1, 1, sizeof(float));
      budget.Storage(S::GRID_MEASURE_COLUMNS, 1, count, sizeof(float));
      break;
    case WorkloadKind::ABSOLUTE:
      budget.Storage(S::ABSOLUTE_MEASURE_CHILDREN, 1, count, sizeof(View));
      break;
  }
}

inline void ArrangeStorage(WorkBudget& budget, WorkloadKind kind, uint32_t count)
{
  using S = WorkDiagnostic::StorageSite;
  switch(kind)
  {
    case WorkloadKind::STACK:
      budget.Storage(S::STACK_ARRANGE_CHILDREN, 1, count, sizeof(View));
      budget.Storage(S::STACK_ARRANGE_WORK, 1, count, sizeof(MeasuredSize));
      break;
    case WorkloadKind::FLEX:
      budget.Storage(S::FLEX_ARRANGE_CHILDREN, 1, count, sizeof(View));
      budget.Storage(S::FLEX_ARRANGE_WORK, 1, count, sizeof(MeasuredSize));
      break;
    case WorkloadKind::GRID:
      budget.Storage(S::GRID_ARRANGE_CHILDREN, 1, count, sizeof(View));
      break;
    case WorkloadKind::ABSOLUTE:
      budget.Storage(S::ABSOLUTE_ARRANGE_CHILDREN, 1, count, sizeof(View));
      break;
  }
}

inline uint64_t RequiredGeometryWrites(const Workload& w)
{
  uint64_t count   = 0;
  auto     compare = [&count](const LayoutRect& actual, const LayoutRect& desired)
  {
    count += actual.x != desired.x;
    count += actual.y != desired.y;
    count += actual.width != desired.width;
    count += actual.height != desired.height;
  };
  compare(LayoutValidation::Bounds(w.root), {0, 0, w.width, w.height});
  for(std::size_t i = 0; i < w.leaves.size(); ++i)
    compare(LayoutValidation::Bounds(w.leaves[i]), w.expected[i]);
  return count;
}

class WorkCapture
{
public:
  static constexpr std::size_t CAPACITY = 131072; // holds the largest warm-pass capture (512-leaf workload: 32 passes x 513 replayed nodes exceeds 16k events); heap-allocated
  WorkCapture()
  : mEvents(new std::array<WorkDiagnostic::Event, CAPACITY>)
  {
  }
  ~WorkCapture()
  {
    if(mActive) WorkDiagnostic::EndCapture();
    WorkDiagnostic::ClearRegisteredNodes();
  }
  bool Begin(const Workload& w, uint64_t epoch)
  {
    WorkDiagnostic::ClearRegisteredNodes();
    mRegistered = WorkDiagnostic::RegisterNode(w.root, 1);
    for(uint32_t i = 0; i < w.leaves.size(); ++i)
      mRegistered = WorkDiagnostic::RegisterNode(w.leaves[i], i + 2) && mRegistered;
    mNodes   = static_cast<uint32_t>(w.leaves.size()) + 1;
    mEpoch   = epoch;
    mActive  = mRegistered && WorkDiagnostic::BeginCapture(mEvents->data(), mEvents->size(), epoch);
    mStarted = mActive;
    return mActive;
  }
  void End()
  {
    if(mActive) mResult = WorkDiagnostic::EndCapture();
    mActive = false;
  }
  void Check(Run& run, const WorkBudget& budget) const
  {
    using K = WorkDiagnostic::EventKind;
    const std::array<K, WORK_EVENT_KINDS>           kinds{{K::MEASURE_ENTER, K::MEASURE_HIT, K::MEASURE_PRODUCER, K::MEASURE_PUBLISH,
                                                           K::ARRANGE_ENTER, K::ARRANGE_HIT, K::ARRANGE_PRODUCER, K::ARRANGE_PUBLISH, K::REPLAY_VISIT, K::GEOMETRY_WRITE,
                                                           K::WAKE_REQUEST, K::ROOT_DRAIN, K::PARKED_REQUEST, K::INVALIDATE_MEASURE, K::ANCESTOR_VISIT}};
    const std::array<const char*, WORK_EVENT_KINDS> names{{"measure.enter", "measure.hit", "measure.producer", "measure.publish",
                                                           "arrange.enter", "arrange.hit", "arrange.producer", "arrange.publish", "replay.visit", "geometry.write",
                                                           "wake.request", "root.drain", "park.request", "invalidate.measure", "ancestor.visit"}};
    const std::array<uint64_t, WORK_EVENT_KINDS>    expected{{budget.measureEnter, budget.measureHit, budget.measureProducer, budget.measurePublish,
                                                              budget.arrangeEnter, budget.arrangeHit, budget.arrangeProducer, budget.arrangePublish, budget.replay, budget.writes,
                                                              budget.wake, budget.rootDrain, budget.parked, budget.invalidateMeasure, budget.ancestorVisits}};
    std::array<uint64_t, WORK_EVENT_KINDS>          actual{};
    std::array<StorageBudget, WORK_STORAGE_RULES>   stored{};
    uint64_t                                        storageTotal = 0, expectedStorageTotal = 0, unknownStorage = 0;
    bool                                            protocol = mResult.epoch == mEpoch && mResult.totalEvents == mResult.count;
    bool                                            nodes    = true;
    for(const auto& rule : budget.storage) expectedStorageTotal += rule.calls;
    for(std::size_t i = 0; i < mResult.count; ++i)
    {
      const auto& event = (*mEvents)[i];
      protocol &= event.epoch == mEpoch && event.sequence == i + 1;
      for(std::size_t k = 0; k < kinds.size(); ++k)
      {
        if(event.kind != kinds[k]) continue;
        // Node-bound kinds count registered workload nodes only; the on-screen stage and the
        // launcher views around the workload are not part of the budget.
        if((k <= 9 || k >= 13) && event.nodeId == 0) continue;
        ++actual[k];
        if(k <= 9 || k >= 13) nodes &= event.nodeId <= mNodes;
      }
      if(event.kind == K::SNAPSHOT_ALLOCATION && event.nodeId != 0)
      {
        ++storageTotal;
        bool known = false;
        for(std::size_t slot = 0; slot < budget.usedStorage; ++slot)
        {
          if(event.detail != static_cast<uint32_t>(budget.storage[slot].site)) continue;
          known = true;
          ++stored[slot].calls;
          const bool finite = std::isfinite(event.value0) && std::isfinite(event.value1) && event.value0 >= 0 && event.value1 >= 0 &&
                              std::floor(event.value0) == event.value0 && std::floor(event.value1) == event.value1 &&
                              event.value0 < static_cast<float>(std::numeric_limits<uint64_t>::max()) &&
                              event.value1 < static_cast<float>(std::numeric_limits<uint64_t>::max());
          protocol &= finite;
          if(finite)
          {
            stored[slot].elements += static_cast<uint64_t>(event.value0);
            stored[slot].bytes += static_cast<uint64_t>(event.value1);
          }
          nodes &= event.nodeId == 1;
        }
        if(!known) ++unknownStorage;
      }
    }
    run.Truth("work.capture.started", mStarted && mRegistered);
    run.Truth("work.capture.complete", !mResult.active && !mResult.overflow);
    run.Truth("work.capture.epoch-sequence", protocol);
    run.Truth("work.capture.node-identities", nodes);
    for(std::size_t k = 0; k < kinds.size(); ++k) run.Equal((std::string("work.") + names[k]).c_str(), actual[k], expected[k]);
    run.Equal("work.storage.total-calls", storageTotal, expectedStorageTotal);
    run.Equal("work.storage.unexpected-sites", unknownStorage, 0);
    for(std::size_t slot = 0; slot < budget.storage.size(); ++slot)
    {
      const std::string id = "work.storage.slot" + std::to_string(slot) + ".site" + std::to_string(static_cast<uint32_t>(budget.storage[slot].site));
      run.Equal((id + ".calls").c_str(), stored[slot].calls, budget.storage[slot].calls);
      run.Equal((id + ".elements").c_str(), stored[slot].elements, budget.storage[slot].elements);
      run.Equal((id + ".bytes").c_str(), stored[slot].bytes, budget.storage[slot].bytes);
    }
  }

private:
  std::unique_ptr<std::array<WorkDiagnostic::Event, CAPACITY>> mEvents;
  WorkDiagnostic::CaptureResult                                mResult{};
  uint64_t                                                     mEpoch{0};
  uint32_t                                                     mNodes{0};
  bool                                                         mRegistered{false}, mStarted{false}, mActive{false};
};
} // namespace LayoutValidation
