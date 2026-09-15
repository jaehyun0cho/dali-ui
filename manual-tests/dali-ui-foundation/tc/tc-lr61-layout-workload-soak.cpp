/* Copyright (c) 2026 Samsung Electronics Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */
#include "layout-validation-workloads.h"
using namespace LayoutValidation;
namespace
{
class TcLr61 : public Case
{
public:
  TcLr61()
  : Case("LR61", "Workload soak")
  {
  }
  std::vector<Scenario> BuildScenarios() override
  {
    std::vector<Scenario> result;
    for(auto kind : {WorkloadKind::STACK, WorkloadKind::FLEX, WorkloadKind::GRID, WorkloadKind::ABSOLUTE})
      result.push_back(Single(KindName(kind), "256 cold/repeat/corruption-repair cycles with full geometry comparison", 768, [kind](Run& r)
      {
        r.RequireExternalVerification("LR61 requires ten real TC entries, cleanup-end evidence, RSS/PSS observations and allocation-stack profiling; see the TC MD");
        auto w = FlatWorkload(kind, 128);
        for(uint32_t cycle = 0; cycle < 256; ++cycle)
        {
          Settle(w);
          const uint32_t index = (cycle * 37u) % 128u;
          w.leaves[index].SetProperty(Actor::Property::POSITION_X, -123.25f);
          w.leaves[index].SetProperty(Actor::Property::SIZE_WIDTH, 1.f);
          w.root.Arrange(LayoutRect(0, 0, w.width, w.height));
          VerifyWorkload(r, w, Id("cycle", cycle).c_str());
          if(cycle % 16 == 0)
          {
            w.root.InvalidateMeasure();
            w.root.InvalidateArrange();
          }
        }
      }));
    for(uint32_t depth : {8u, 64u, 192u})
      result.push_back(Single(("depth-" + std::to_string(depth)).c_str(), "Nested padding: root intrinsic size and every local rectangle", 2u + 4u * depth, [depth](Run& r)
      {
        r.RequireExternalVerification("LR61 requires ten real TC entries, cleanup-end evidence, RSS/PSS observations and allocation-stack profiling; see the TC MD");
        View                                     inner = Leaf(8, 16);
        std::vector<std::pair<View, LayoutRect>> expected;
        for(uint32_t i = 0; i < depth; ++i)
        {
          auto parent = StackLayout::New();
          parent.SetPadding(Insets(1, 1, 1, 1));
          parent.SetRequestedWidth(WRAP_CONTENT);
          parent.SetRequestedHeight(WRAP_CONTENT);
          parent.Add(inner);
          expected.push_back({inner, LayoutRect(1, 1, 8.f + 2 * i, 16.f + 2 * i)});
          inner = parent;
        }
        r.Size("deep.root", inner.Measure(1024, 1024), MeasuredSize(8.f + 2 * depth, 16.f + 2 * depth));
        inner.Arrange(LayoutRect(0, 0, 8.f + 2 * depth, 16.f + 2 * depth));
        for(uint32_t i = 0; i < depth; ++i) r.Rect(Id("deep.child", i).c_str(), expected[i].first, expected[i].second);
      }));
    return result;
  }
};
} //namespace
REGISTER_MANUAL_TEST(TcLr61)
