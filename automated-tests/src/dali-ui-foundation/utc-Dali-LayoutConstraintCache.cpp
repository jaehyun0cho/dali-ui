/*
 * Copyright (c) 2026 Samsung Electronics Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <dali-ui-foundation/dali-ui-foundation.h>
#include <dali-ui-foundation/public-api/layouts/grid-layout-manager.h>
#include <dali-ui-foundation/public-api/layouts/stack-layout-manager.h>
#include <dali-ui-test-suite-utils.h>
#include <dali.h>
#include <dali/devel-api/object/type-registry.h>

#include <cmath>
#include <cstdio>
#include <functional>
#include <limits>
#include <utility>

using namespace Dali;
using namespace Dali::Ui;

namespace
{
class ScaleScope
{
public:
  explicit ScaleScope(float scale)
  : mManager(UiScaleManager::Get()),
    mScale(mManager.GetScale()),
    mScalable(mManager.IsScalable())
  {
    mManager.SetScalable(true);
    mManager.SetScale(scale);
  }

  ~ScaleScope()
  {
    mManager.SetScale(mScale);
    mManager.SetScalable(mScalable);
  }

private:
  UiScaleManager mManager;
  float          mScale;
  bool           mScalable;
};

// Only this test subclass stores counters. Both producers delegate to the
// default implementation, including its child traversal and cache behavior.
class ConstraintCounterViewImpl : public ViewImpl
{
public:
  static IntrusivePtr<ConstraintCounterViewImpl> New()
  {
    return new ConstraintCounterViewImpl();
  }

  void Reset()
  {
    measures = 0;
    arranges = 0;
  }

  int                   measures{0};
  int                   arranges{0};
  MeasuredSize          lastConstraint{0.0f, 0.0f};
  LayoutRect            lastSlot{0.0f, 0.0f, 0.0f, 0.0f};
  std::function<void()> nextArrangeAction;

protected:
  MeasuredSize OnMeasure(float width, float height) override
  {
    ++measures;
    lastConstraint = MeasuredSize(width, height);
    return ViewImpl::OnMeasure(width, height);
  }

  LayoutRect OnArrange(const LayoutRect& bounds) override
  {
    ++arranges;
    lastSlot          = bounds;
    auto action       = std::move(nextArrangeAction);
    nextArrangeAction = {};
    if(action)
    {
      action();
    }
    return ViewImpl::OnArrange(bounds);
  }
};

Dali::TypeRegistration constraintCounterRegistration(
  typeid(ConstraintCounterViewImpl), typeid(Dali::Ui::View), nullptr);

View CounterView(float width, float height = 20.0f)
{
  auto impl = ConstraintCounterViewImpl::New();
  View view(*impl);
  view.SetRequestedWidth(width);
  view.SetRequestedHeight(height);
  return view;
}

ConstraintCounterViewImpl& Counter(View view)
{
  return static_cast<ConstraintCounterViewImpl&>(GetImpl(view));
}

struct LayoutPairFixture
{
  View parent{CounterView(100.0f)};
  View child{CounterView(MATCH_PARENT)};
  View sibling{CounterView(1.0f)};

  LayoutPairFixture()
  {
    parent.SetPadding(3.0f, 3.0f, 0.0f, 0.0f);
    parent.Add(child);
    parent.Add(sibling);
  }

  void Pass()
  {
    const float scale = UiScaleManager::Get().GetScale();
    parent.Measure(100.0f * scale, 20.0f * scale);
    parent.Arrange(LayoutRect(0.0f, 0.0f, 100.0f * scale, 20.0f * scale));
  }

  void Reset()
  {
    Counter(parent).Reset();
    Counter(child).Reset();
    Counter(sibling).Reset();
  }
};

void PrintCounts(const char* label, const LayoutPairFixture& pair)
{
  const auto& parent = Counter(pair.parent);
  const auto& child  = Counter(pair.child);
  std::printf("layout-constraint %s parent=%d/%d child=%d/%d input=%.9g/%.9g slot=%.9g/%.9g\n",
              label, parent.measures, parent.arranges, child.measures, child.arranges,
              child.lastConstraint.width, child.lastConstraint.height,
              child.lastSlot.width, child.lastSlot.height);
}

FlexLayout WrappingPair(float childWidth = 50.0f)
{
  FlexLayout layout = FlexLayout::New();
  layout.SetDirection(FlexDirection::ROW);
  layout.SetWrap(FlexWrap::WRAP);
  layout.SetRequestedWidth(WRAP_CONTENT);
  layout.SetRequestedHeight(WRAP_CONTENT);
  for(int i = 0; i < 2; ++i)
  {
    View child = View::New();
    child.SetRequestedWidth(childWidth);
    child.SetRequestedHeight(20.0f);
    layout.Add(child);
  }
  return layout;
}

View ArrangeInsets(const Insets& padding, const Insets& margin, float width)
{
  View parent = CounterView(100.0f);
  View child  = CounterView(MATCH_PARENT);
  parent.SetPadding(padding);
  child.SetMargin(margin);
  parent.Add(child);
  parent.Arrange(LayoutRect(0.0f, 0.0f, width, 22.0f));
  return child;
}
} // namespace

void utc_dali_layoutconstraintcache_startup(void)
{
  test_return_value = TET_UNDEF;
}

void utc_dali_layoutconstraintcache_cleanup(void)
{
  test_return_value = TET_PASS;
}

// Run this unchanged against each real production build. It reports observed
// work without embedding a model of alternative production algorithms.
int UtcDaliLayoutConstraintCacheProducerDiagnosticP(void)
{
  UiTestApplication application;
  ScaleScope        scale(1.1f);
  LayoutPairFixture pair;
  pair.Pass();
  PrintCounts("initial", pair);

  pair.Reset();
  pair.child.InvalidateMeasure();
  pair.Pass();
  PrintCounts("dirty-child", pair);

  pair.Reset();
  pair.sibling.SetRequestedWidth(2.0f);
  pair.Pass();
  PrintCounts("clean-sibling", pair);

  pair.Reset();
  for(int i = 0; i < 16; ++i)
  {
    pair.sibling.SetRequestedWidth((i % 2) ? 2.0f : 1.0f);
    pair.Pass();
  }
  PrintCounts("clean-sibling-16", pair);

  pair.Reset();
  pair.Pass();
  PrintCounts("unchanged", pair);
  DALI_TEST_CHECK(Counter(pair.parent).measures == 0);
  DALI_TEST_CHECK(Counter(pair.parent).arranges == 0);
  END_TEST;
}

int UtcDaliLayoutConstraintCacheWarmMatchesColdWrapP(void)
{
  UiTestApplication  application;
  ScaleScope         scale(1.0f);
  FlexLayout         warm = WrappingPair();
  const MeasuredSize wide = warm.Measure(100.0f, 100.0f);
  DALI_TEST_CHECK(wide.width == 100.0f && wide.height == 20.0f);

  const float        narrower = 99.99951171875f;
  const MeasuredSize reused   = warm.Measure(narrower, 100.0f);
  FlexLayout         cold     = WrappingPair();
  const MeasuredSize fresh    = cold.Measure(narrower, 100.0f);
  DALI_TEST_CHECK(fresh.width == 50.0f && fresh.height == 40.0f);
  DALI_TEST_CHECK(reused.width == fresh.width && reused.height == fresh.height);
  END_TEST;
}

int UtcDaliLayoutConstraintCacheSharedBudgetProducerCountsP(void)
{
  UiTestApplication application;
  ScaleScope        scale(1.1f);
  LayoutPairFixture pair;
  pair.Pass();

  pair.Reset();
  pair.child.InvalidateMeasure();
  pair.Pass();
  DALI_TEST_CHECK(Counter(pair.parent).measures == 1 && Counter(pair.parent).arranges == 1);
  DALI_TEST_CHECK(Counter(pair.child).measures == 1 && Counter(pair.child).arranges == 1);

  pair.Reset();
  for(int i = 0; i < 16; ++i)
  {
    pair.sibling.SetRequestedWidth((i % 2) ? 1.0f : 2.0f);
    pair.Pass();
  }
  DALI_TEST_CHECK(Counter(pair.parent).measures == 16 && Counter(pair.parent).arranges == 16);
  DALI_TEST_CHECK(Counter(pair.child).measures == 0 && Counter(pair.child).arranges == 0);

  pair.Reset();
  pair.Pass();
  DALI_TEST_CHECK(Counter(pair.parent).measures == 0 && Counter(pair.parent).arranges == 0);
  DALI_TEST_CHECK(Counter(pair.child).measures == 0 && Counter(pair.child).arranges == 0);
  END_TEST;
}

int UtcDaliLayoutConstraintCacheActualSlotRemainsUnclampedP(void)
{
  UiTestApplication application;
  ScaleScope        scale(1.0f);
  View              parent = CounterView(MATCH_PARENT);
  View              child  = CounterView(MATCH_PARENT);
  parent.SetMaximumWidth(80.0f);
  parent.Add(child);
  parent.Measure(100.0f, 20.0f);
  DALI_TEST_CHECK(Counter(child).lastConstraint.width == 80.0f);
  parent.Arrange(LayoutRect(0.0f, 0.0f, 100.0f, 20.0f));
  DALI_TEST_CHECK(Counter(child).lastSlot.width == 100.0f);
  DALI_TEST_CHECK(Counter(child).lastConstraint.width == 100.0f);

  View fixedParent = CounterView(80.0f);
  View fixedChild  = CounterView(MATCH_PARENT);
  fixedParent.Add(fixedChild);
  fixedParent.Measure(100.0f, 20.0f);
  fixedParent.Arrange(LayoutRect(0.0f, 0.0f, 100.0f, 20.0f));
  DALI_TEST_CHECK(Counter(fixedChild).lastSlot.width == 100.0f);
  END_TEST;
}

int UtcDaliLayoutConstraintCacheFractionalInsetsAndNegativeFallbackP(void)
{
  UiTestApplication application;
  ScaleScope        scale(1.1f);
  const float       width = 100.0f * 1.1f;

  View fractional = ArrangeInsets(Insets(0.25f, 1.5f, 0.0f, 0.0f),
                                  Insets(0.125f, 0.375f, 0.0f, 0.0f), width);
  // This value distinguishes summed natural insets from separate scaled subtraction.
  DALI_TEST_CHECK(Counter(fractional).lastSlot.width == 107.5250015258789f);

  View negativePadding = ArrangeInsets(Insets(-0.125f, 1.5f, 0.0f, 0.0f),
                                       Insets(0.125f, 0.375f, 0.0f, 0.0f), width);
  DALI_TEST_CHECK(Counter(negativePadding).lastSlot.width == 107.93749237060547f);

  View negativeMargin = ArrangeInsets(Insets(0.25f, 1.5f, 0.0f, 0.0f),
                                      Insets(-0.125f, 0.375f, 0.0f, 0.0f), width);
  DALI_TEST_CHECK(Counter(negativeMargin).lastSlot.width == 107.79999542236328f);
  END_TEST;
}

int UtcDaliLayoutConstraintCacheNonFiniteInsetsFallbackP(void)
{
  UiTestApplication application;
  ScaleScope        scale(1.1f);
  const float       exceptional[] = {std::numeric_limits<float>::quiet_NaN(),
                                     std::numeric_limits<float>::infinity(),
                                     std::numeric_limits<float>::max()};
  for(float value : exceptional)
  {
    // End-only exceptional insets leave x finite. Legacy subtraction clamps
    // the width to zero, so the child's arrange producer still runs.
    View padding = ArrangeInsets(Insets(0.0f, value, 0.0f, 0.0f), Insets(), 110.0f);
    DALI_TEST_CHECK(Counter(padding).arranges == 1 && Counter(padding).lastSlot.width == 0.0f);
    View margin = ArrangeInsets(Insets(), Insets(0.0f, value, 0.0f, 0.0f), 110.0f);
    DALI_TEST_CHECK(Counter(margin).arranges == 1 && Counter(margin).lastSlot.width == 0.0f);
  }
  END_TEST;
}

int UtcDaliLayoutConstraintCacheNormalizationOverflowFallbackP(void)
{
  UiTestApplication application;
  ScaleScope        scale(0.5f);
  View              parent = CounterView(100.0f);
  View              child  = CounterView(MATCH_PARENT);
  parent.Add(child);
  const float width = std::numeric_limits<float>::max();
  parent.Arrange(LayoutRect(0.0f, 0.0f, width, 10.0f));
  DALI_TEST_CHECK(Counter(child).arranges == 1);
  DALI_TEST_CHECK(Counter(child).lastSlot.width == width);
  END_TEST;
}

int UtcDaliLayoutConstraintCacheRoundTripNaturalBudgetP(void)
{
  UiTestApplication application;
  ScaleScope        scale(1.1f);
  View              parent = CounterView(MATCH_PARENT);
  View              child  = CounterView(MATCH_PARENT);
  parent.SetPadding(1.0f, 1.0f, 0.0f, 0.0f);
  parent.Add(child);
  const float width = 922.50830078125f;
  parent.Measure(width, 22.0f);
  DALI_TEST_CHECK(Counter(child).measures == 1);
  parent.Arrange(LayoutRect(0.0f, 0.0f, width, 22.0f));
  DALI_TEST_CHECK(Counter(child).measures == 1);
  DALI_TEST_CHECK(Counter(child).arranges == 1);
  END_TEST;
}

int UtcDaliLayoutConstraintCacheFixedDescendantReusesMeasurementP(void)
{
  UiTestApplication application;
  ScaleScope        scale(1.1f);
  LayoutPairFixture pair;
  View              fixed = CounterView(40.0f);
  View              leaf  = CounterView(MATCH_PARENT);
  fixed.Add(leaf);
  pair.child.Add(fixed);
  pair.Pass();
  const LayoutRect fixedSlot = Counter(fixed).lastSlot;
  Counter(fixed).Reset();
  Counter(leaf).Reset();

  // Change the fixed view's actual cache key, so the descendant reuse cannot
  // be explained by a cache hit at an ancestor that skipped its producer.
  fixed.Measure(80.0f * 1.1f, 20.0f * 1.1f);
  DALI_TEST_CHECK(Counter(fixed).measures == 1);
  DALI_TEST_CHECK(Counter(leaf).measures == 0);
  fixed.Arrange(fixedSlot);
  DALI_TEST_CHECK(Counter(fixed).arranges == 1);
  DALI_TEST_CHECK(Counter(leaf).measures == 0 && Counter(leaf).arranges == 0);
  DALI_TEST_CHECK(Counter(leaf).lastSlot.width == 44.0f);
  END_TEST;
}

int UtcDaliLayoutConstraintCacheColdWrapAndMixedAxisSlotP(void)
{
  UiTestApplication application;
  ScaleScope        scale(1.1f);
  View              parent = CounterView(100.0f);
  FlexLayout        child  = WrappingPair(47.0f);
  parent.SetPadding(3.0f, 3.0f, 0.0f, 0.0f);
  parent.Add(child);

  parent.Measure(110.0f, 22.0f);
  DALI_TEST_CHECK(child.GetMeasuredSize().width == 103.4000015258789f);
  DALI_TEST_CHECK(child.GetMeasuredSize().height == 22.0f);
  parent.Arrange(LayoutRect(0.0f, 0.0f, 110.0f, 22.0f));
  DALI_TEST_CHECK(child.GetProperty<float>(Actor::Property::SIZE_WIDTH) == 103.4000015258789f);
  DALI_TEST_CHECK(child.GetProperty<float>(Actor::Property::SIZE_HEIGHT) == 22.0f);

  // The main axis now consumes the available slot. Repeating its measurement
  // during arrange must preserve the one-row result produced by the cold pass.
  child.SetRequestedWidth(MATCH_PARENT);
  parent.Measure(110.0f, 22.0f);
  DALI_TEST_CHECK(child.GetMeasuredSize().height == 22.0f);
  parent.Arrange(LayoutRect(0.0f, 0.0f, 110.0f, 22.0f));
  DALI_TEST_CHECK(child.GetMeasuredSize().height == 22.0f);
  DALI_TEST_CHECK(child.GetProperty<float>(Actor::Property::SIZE_HEIGHT) == 22.0f);
  END_TEST;
}

int UtcDaliLayoutConstraintCacheHeightBudgetAndArrangeOnlyChangeP(void)
{
  UiTestApplication application;
  ScaleScope        scale(1.1f);
  View              parent  = CounterView(20.0f, 100.0f);
  View              child   = CounterView(20.0f, MATCH_PARENT);
  View              sibling = CounterView(1.0f, 1.0f);
  parent.SetPadding(0.0f, 0.0f, 3.0f, 3.0f);
  parent.Add(child);
  parent.Add(sibling);
  const LayoutRect originalSlot(0.0f, 0.0f, 22.0f, 110.0f);
  auto             pass = [&]()
  {
    parent.Measure(22.0f, 110.0f);
    parent.Arrange(originalSlot);
  };
  pass();

  Counter(parent).Reset();
  Counter(child).Reset();
  child.InvalidateMeasure();
  pass();
  DALI_TEST_CHECK(Counter(parent).measures == 1 && Counter(parent).arranges == 1);
  DALI_TEST_CHECK(Counter(child).measures == 1 && Counter(child).arranges == 1);

  Counter(parent).Reset();
  Counter(child).Reset();
  sibling.SetRequestedWidth(2.0f);
  pass();
  DALI_TEST_CHECK(Counter(parent).measures == 1 && Counter(parent).arranges == 1);
  DALI_TEST_CHECK(Counter(child).measures == 0 && Counter(child).arranges == 0);

  const float previousHeight = Counter(child).lastSlot.height;
  Counter(parent).Reset();
  Counter(child).Reset();
  parent.Arrange(LayoutRect(0.0f, 0.0f, 22.0f, 109.99951171875f));
  DALI_TEST_CHECK(Counter(parent).measures == 0 && Counter(parent).arranges == 1);
  DALI_TEST_CHECK(Counter(child).measures == 1 && Counter(child).arranges == 1);
  DALI_TEST_CHECK(Counter(child).lastSlot.height < previousHeight);
  END_TEST;
}

int UtcDaliLayoutConstraintCacheScaledWideBudgetReusesCleanChildP(void)
{
  UiTestApplication application;
  for(float value : {2.2f, 0.9f})
  {
    ScaleScope scale(value);
    View       parent  = CounterView(1080.0f);
    View       child   = CounterView(MATCH_PARENT);
    View       sibling = CounterView(1.0f);
    parent.SetPadding(12.0f, 12.0f, 0.0f, 0.0f);
    parent.Add(child);
    parent.Add(sibling);
    const LayoutRect slot(0.0f, 0.0f, 1080.0f * value, 20.0f * value);
    parent.Measure(slot.width, slot.height);
    parent.Arrange(slot);

    Counter(parent).Reset();
    Counter(child).Reset();
    sibling.SetRequestedWidth(2.0f);
    parent.Measure(slot.width, slot.height);
    parent.Arrange(slot);
    DALI_TEST_CHECK(Counter(parent).measures == 1 && Counter(parent).arranges == 1);
    DALI_TEST_CHECK(Counter(child).measures == 0 && Counter(child).arranges == 0);
  }
  END_TEST;
}

int UtcDaliLayoutConstraintCacheFixedOperandBothAxesP(void)
{
  UiTestApplication application;
  bool              success = true;
  auto              check   = [&](bool condition)
  { success = condition && success; };
  const struct
  {
    float extent;
    float scale;
  } cases[] = {{414.0f, 0.9f}, {768.0f, 0.9f}, {3840.0f, 1.2f}};

  for(const auto& sample : cases)
  {
    ScaleScope scale(sample.scale);
    for(bool vertical : {false, true})
    {
      View parent  = CounterView(vertical ? 20.0f : sample.extent, vertical ? sample.extent : 20.0f);
      View child   = CounterView(vertical ? 20.0f : MATCH_PARENT, vertical ? MATCH_PARENT : 20.0f);
      View sibling = CounterView(1.0f);
      parent.SetPadding(vertical ? Insets(0.0f, 0.0f, 3.0f, 3.0f) : Insets(3.0f, 3.0f, 0.0f, 0.0f));
      parent.Add(child);
      parent.Add(sibling);
      const LayoutRect slot(0.0f, 0.0f, parent.GetRequestedWidth() * sample.scale,
                            parent.GetRequestedHeight() * sample.scale);
      auto             pass = [&]()
      {
        parent.Measure(slot.width, slot.height);
        parent.Arrange(slot);
      };

      const MeasuredSize cold = parent.Measure(slot.width, slot.height);
      check(cold.width == slot.width && cold.height == slot.height);
      check(Counter(child).measures == 1);
      parent.Arrange(slot);
      check(Counter(child).measures == 1 && Counter(child).arranges == 1);
      const int        coldCounts[] = {Counter(parent).measures, Counter(parent).arranges, Counter(child).measures, Counter(child).arranges};
      const LayoutRect settled      = Counter(child).lastSlot;

      Counter(parent).Reset();
      Counter(child).Reset();
      child.InvalidateMeasure();
      pass();
      check(Counter(parent).measures == 1 && Counter(parent).arranges == 1);
      check(Counter(child).measures == 1 && Counter(child).arranges == 1);
      const int dirtyCounts[] = {Counter(parent).measures, Counter(parent).arranges, Counter(child).measures, Counter(child).arranges};

      Counter(parent).Reset();
      Counter(child).Reset();
      for(int i = 0; i < 16; ++i)
      {
        sibling.SetRequestedWidth((i % 2) ? 1.0f : 2.0f);
        pass();
      }
      std::printf("layout-fixed-operand extent=%.9g scale=%.9g axis=%s cold-parent=%d/%d cold-child=%d/%d dirty-parent=%d/%d dirty-child=%d/%d sibling16-parent=%d/%d sibling16-child=%d/%d\n",
                  sample.extent, sample.scale, vertical ? "height" : "width",
                  coldCounts[0], coldCounts[1], coldCounts[2], coldCounts[3],
                  dirtyCounts[0], dirtyCounts[1], dirtyCounts[2], dirtyCounts[3],
                  Counter(parent).measures, Counter(parent).arranges,
                  Counter(child).measures, Counter(child).arranges);
      check(Counter(parent).measures == 16 && Counter(parent).arranges == 16);
      check(Counter(child).measures == 0 && Counter(child).arranges == 0);
      check(Counter(child).lastSlot.width == settled.width && Counter(child).lastSlot.height == settled.height);
      check(parent.GetProperty<float>(Actor::Property::SIZE_WIDTH) == slot.width);
      check(parent.GetProperty<float>(Actor::Property::SIZE_HEIGHT) == slot.height);
    }
  }
  DALI_TEST_CHECK(success);
  END_TEST;
}

int UtcDaliLayoutConstraintCacheFixedOperandDifferentChildScaleP(void)
{
  UiTestApplication application;
  ScaleScope        scale(0.9f);
  View              parent  = CounterView(414.0f);
  View              child   = CounterView(MATCH_PARENT, MATCH_PARENT);
  View              sibling = CounterView(1.0f);
  child.SetUiScalePolicy(UiScalePolicy::DISABLED);
  child.SetMargin(0.125f, 0.375f, 0.0f, 0.0f);
  parent.SetPadding(3.0f, 3.0f, 0.0f, 0.0f);
  parent.Add(child);
  parent.Add(sibling);
  const LayoutRect slot(0.0f, 0.0f, 414.0f * 0.9f, 18.0f);
  parent.Measure(slot.width, slot.height);
  const MeasuredSize coldInput = Counter(child).lastConstraint;
  DALI_TEST_CHECK(GetImpl(child).GetEffectiveScale() == 1.0f);
  parent.Arrange(slot);
  DALI_TEST_CHECK(Counter(child).measures == 1 && Counter(child).arranges == 1);
  // With the child's scale disabled its producer input is also its visual
  // budget. Compare the actual cold call against the actual arranged slot.
  DALI_TEST_CHECK(Counter(child).lastSlot.width == coldInput.width);
  DALI_TEST_CHECK(Counter(child).lastSlot.height == coldInput.height);

  Counter(parent).Reset();
  Counter(child).Reset();
  for(int i = 0; i < 16; ++i)
  {
    sibling.SetRequestedWidth((i % 2) ? 1.0f : 2.0f);
    parent.Measure(slot.width, slot.height);
    parent.Arrange(slot);
  }
  DALI_TEST_CHECK(Counter(parent).measures == 16 && Counter(parent).arranges == 16);
  DALI_TEST_CHECK(Counter(child).measures == 0 && Counter(child).arranges == 0);
  END_TEST;
}

int UtcDaliLayoutConstraintCacheFixedOperandRespectsActualSlotP(void)
{
  // Preserve the existing cold policy: a fixed request supplies the child budget
  // before the parent's measured result is clamped. Arrange uses the actual slot,
  // even when it differs from that result; sharing must not override the slot.
  UiTestApplication application;
  ScaleScope        scale(1.0f);
  const struct
  {
    float requested;
    float maximum;
    float slot;
    int   childMeasures;
  } cases[] = {{100.0f, 80.0f, 80.0f, 2},
               {100.0f, 80.0f, 100.0f, 1},
               {80.0f, std::numeric_limits<float>::max(), 100.0f, 2}};
  for(const auto& sample : cases)
  {
    View parent = CounterView(sample.requested);
    View child  = CounterView(MATCH_PARENT);
    parent.SetMaximumWidth(sample.maximum);
    parent.Add(child);
    const MeasuredSize cold = parent.Measure(100.0f, 20.0f);
    DALI_TEST_CHECK(cold.width == 80.0f);
    DALI_TEST_CHECK(Counter(child).lastConstraint.width == sample.requested);
    const LayoutRect actual = parent.Arrange(LayoutRect(0.0f, 0.0f, sample.slot, 20.0f));
    DALI_TEST_CHECK(actual.width == sample.slot);
    DALI_TEST_CHECK(parent.GetProperty<float>(Actor::Property::SIZE_WIDTH) == sample.slot);
    DALI_TEST_CHECK(Counter(child).lastSlot.width == sample.slot);
    DALI_TEST_CHECK(Counter(child).measures == sample.childMeasures);
  }
  END_TEST;
}

int UtcDaliLayoutConstraintCacheFixedOperandDifferentSlotsP(void)
{
  UiTestApplication application;
  ScaleScope        scale(0.9f);
  View              parent = CounterView(414.0f);
  View              child  = CounterView(MATCH_PARENT);
  parent.SetPadding(3.0f, 3.0f, 0.0f, 0.0f);
  parent.Add(child);
  const float original = 414.0f * 0.9f;
  parent.Measure(original, 18.0f);
  parent.Arrange(LayoutRect(0.0f, 0.0f, original, 18.0f));
  Counter(parent).Reset();

  for(float changed : {std::nextafter(original, 0.0f),
                       std::nextafter(original, std::numeric_limits<float>::infinity()),
                       original - 1.0f})
  {
    // The MP parent supplies the existing actual-slot behavior without a
    // fixed requested operand. Do not assume neighboring slots yield distinct
    // normalized child keys or force a measure miss for every neighbor.
    View reference      = CounterView(MATCH_PARENT);
    View referenceChild = CounterView(MATCH_PARENT);
    reference.SetPadding(3.0f, 3.0f, 0.0f, 0.0f);
    reference.Add(referenceChild);
    const LayoutRect slot(0.0f, 0.0f, changed, 18.0f);
    reference.Measure(slot.width, slot.height);
    reference.Arrange(slot);
    parent.Arrange(slot);
    DALI_TEST_CHECK(Counter(child).lastSlot.width == Counter(referenceChild).lastSlot.width);
    DALI_TEST_CHECK(Counter(child).lastSlot.height == Counter(referenceChild).lastSlot.height);
    DALI_TEST_CHECK(parent.GetProperty<float>(Actor::Property::SIZE_WIDTH) == changed);
  }
  DALI_TEST_CHECK(Counter(parent).measures == 0 && Counter(parent).arranges == 3);
  END_TEST;
}

int UtcDaliLayoutConstraintCacheFixedOperandParentSnapshotP(void)
{
  UiTestApplication application;
  ScaleScope        scale(0.9f);
  const LayoutRect  original(0.0f, 0.0f, 414.0f * 0.9f, 414.0f * 0.9f);
  View              reference      = CounterView(414.0f, 414.0f);
  View              referenceChild = CounterView(MATCH_PARENT, MATCH_PARENT);
  reference.SetPadding(3.0f, 3.0f, 3.0f, 3.0f);
  reference.Add(referenceChild);
  reference.Measure(original.width, original.height);
  reference.Arrange(original);
  const LayoutRect expected = Counter(referenceChild).lastSlot;

  View parent = CounterView(414.0f, 414.0f);
  View first  = CounterView(1.0f, 1.0f);
  View child  = CounterView(MATCH_PARENT, MATCH_PARENT);
  parent.SetPadding(3.0f, 3.0f, 3.0f, 3.0f);
  parent.Add(first);
  parent.Add(child);
  parent.Measure(original.width, original.height);
  // Deliberately change layout state during a pass to test defensive capture;
  // applications should make these changes at event time. These views are
  // off-scene and Measure/Arrange are called directly, so this fixture does not
  // depend on parked work or a controller wake to exercise the next pass.
  int actions                      = 0;
  Counter(first).nextArrangeAction = [&]()
  {
    ++actions;
    parent.SetRequestedWidth(768.0f);
    parent.SetRequestedHeight(768.0f);
    parent.SetPadding(12.0f, 24.0f, 18.0f, 18.0f);
  };
  // The first child has no MP axis. The first demand for either parent's
  // shared content extent occurs only after its callback changes the parent.
  parent.Arrange(original);
  DALI_TEST_CHECK(actions == 1);
  DALI_TEST_CHECK(Counter(child).lastSlot.x == expected.x && Counter(child).lastSlot.y == expected.y);
  DALI_TEST_CHECK(Counter(child).lastSlot.width == expected.width && Counter(child).lastSlot.height == expected.height);
  DALI_TEST_CHECK(parent.GetProperty<float>(Actor::Property::SIZE_WIDTH) == original.width);
  DALI_TEST_CHECK(parent.GetProperty<float>(Actor::Property::SIZE_HEIGHT) == original.height);

  Counter(parent).Reset();
  const LayoutRect updated(0.0f, 0.0f, 768.0f * 0.9f, 768.0f * 0.9f);
  parent.Measure(updated.width, updated.height);
  parent.Arrange(updated);
  DALI_TEST_CHECK(Counter(parent).measures == 1 && Counter(parent).arranges == 1);
  DALI_TEST_CHECK(Counter(child).lastSlot.width > expected.width && Counter(child).lastSlot.height > expected.height);
  DALI_TEST_CHECK(actions == 1);
  END_TEST;
}

int UtcDaliLayoutConstraintCacheFixedOperandZeroAndSubnormalP(void)
{
  UiTestApplication application;
  {
    ScaleScope scale(0.9f);
    View       parent = CounterView(0.0f);
    View       child  = CounterView(MATCH_PARENT);
    parent.Add(child);
    parent.Measure(0.0f, 18.0f);
    parent.Arrange(LayoutRect(0.0f, 0.0f, -0.0f, 18.0f));
    DALI_TEST_CHECK(Counter(child).measures == 1 && Counter(child).lastSlot.width == 0.0f);
  }

  const float tiny = std::numeric_limits<float>::denorm_min();
  ScaleScope  scale(tiny);
  View        parent = CounterView(0.75f);
  View        child  = CounterView(MATCH_PARENT);
  parent.SetPadding(0.25f, 0.0f, 0.0f, 0.0f);
  parent.Add(child);
  parent.Measure(tiny, 20.0f * tiny);
  parent.Arrange(LayoutRect(0.0f, 0.0f, tiny, 20.0f * tiny));
  // One subnormal visual unit maps to one natural unit at this scale. The
  // candidate must use the cold measurement's zero budget, not slot / scale.
  DALI_TEST_CHECK(Counter(child).lastSlot.width == 0.0f);
  DALI_TEST_CHECK(Counter(child).measures == 1);
  DALI_TEST_CHECK(parent.GetProperty<float>(Actor::Property::SIZE_WIDTH) == tiny);

  View underflow      = CounterView(0.25f);
  View underflowChild = CounterView(MATCH_PARENT);
  underflow.Add(underflowChild);
  underflow.Measure(0.0f, 20.0f * tiny);
  underflow.Arrange(LayoutRect(0.0f, 0.0f, 0.0f, 20.0f * tiny));
  DALI_TEST_CHECK(Counter(underflowChild).lastSlot.width == 0.0f);
  END_TEST;
}

int UtcDaliLayoutConstraintCacheFixedOperandProductOverflowFallbackP(void)
{
  UiTestApplication application;
  ScaleScope        scale(2.0f);
  const float       maximum = std::numeric_limits<float>::max();
  View              parent  = CounterView(maximum);
  View              child   = CounterView(MATCH_PARENT);
  parent.Add(child);
  // The requested product overflows, while this actual slot and its inverse
  // normalization are finite. The existing actual-slot path must survive.
  parent.Arrange(LayoutRect(0.0f, 0.0f, maximum, 40.0f));
  DALI_TEST_CHECK(Counter(child).lastSlot.width == maximum);
  DALI_TEST_CHECK(parent.GetProperty<float>(Actor::Property::SIZE_WIDTH) == maximum);
  END_TEST;
}

namespace
{
struct ManagerDiagnosticCounts
{
  int          measures{0};
  int          arranges{0};
  MeasuredSize input{0.0f, 0.0f};
  LayoutRect   content{0.0f, 0.0f, 0.0f, 0.0f};
};

class DiagnosticStackManager : public StackLayoutManager
{
public:
  explicit DiagnosticStackManager(ManagerDiagnosticCounts& counts)
  : StackLayoutManager(StackOrientation::VERTICAL, 0.0f),
    mCounts(counts)
  {
  }

  MeasuredSize Measure(ViewImpl* view, float width, float height) override
  {
    ++mCounts.measures;
    mCounts.input = MeasuredSize(width, height);
    return StackLayoutManager::Measure(view, width, height);
  }

  void Arrange(ViewImpl* view, const LayoutRect& bounds) override
  {
    ++mCounts.arranges;
    mCounts.content = bounds;
    StackLayoutManager::Arrange(view, bounds);
  }

private:
  ManagerDiagnosticCounts& mCounts;
};

class DiagnosticGridManager : public GridLayoutManager
{
public:
  DiagnosticGridManager(ManagerDiagnosticCounts& counts, const Dali::Vector<GridLength>& rows,
                        const Dali::Vector<GridLength>& columns)
  : GridLayoutManager(rows, columns, 0.0f, 0.0f),
    mCounts(counts)
  {
  }

  MeasuredSize Measure(ViewImpl* view, float width, float height) override
  {
    ++mCounts.measures;
    mCounts.input = MeasuredSize(width, height);
    return GridLayoutManager::Measure(view, width, height);
  }

  void Arrange(ViewImpl* view, const LayoutRect& bounds) override
  {
    ++mCounts.arranges;
    mCounts.content = bounds;
    GridLayoutManager::Arrange(view, bounds);
  }

private:
  ManagerDiagnosticCounts& mCounts;
};

void PrintManagerDiagnostic(const char* kind, const char* phase, float padding, const ManagerDiagnosticCounts& parent, View child)
{
  const auto& item = Counter(child);
  std::printf("layout-manager-mp %s %s padding=%.9g parent-producer=%d/%d child-producer=%d/%d parent-input=%.9g/%.9g content=%.9g/%.9g child-input=%.9g/%.9g slot=%.9g/%.9g\n",
              kind, phase, padding, parent.measures, parent.arranges, item.measures, item.arranges,
              parent.input.width, parent.input.height, parent.content.width, parent.content.height,
              item.lastConstraint.width, item.lastConstraint.height, item.lastSlot.width, item.lastSlot.height);
}
} // namespace

int UtcDaliLayoutConstraintCacheMatchParentManagerDiagnosticP(void)
{
  UiTestApplication application;
  ScaleScope        scale(1.1f);
  for(bool grid : {false, true})
  {
    for(float padding : {0.0f, 1.0f})
    {
      ManagerDiagnosticCounts counts;
      View                    parent = View::New();
      parent.SetRequestedWidth(MATCH_PARENT);
      parent.SetRequestedHeight(20.0f);
      parent.SetPadding(padding, padding, 0.0f, 0.0f);
      View child = CounterView(MATCH_PARENT);
      parent.Add(child);
      if(grid)
      {
        Dali::Vector<GridLength> rows;
        Dali::Vector<GridLength> columns;
        rows.PushBack(GridLength::Star(1.0f));
        columns.PushBack(GridLength::Star(1.0f));
        parent.AttachLayoutManager(Dali::UniquePtr<LayoutManager>(new DiagnosticGridManager(counts, rows, columns)));
      }
      else
      {
        parent.AttachLayoutManager(Dali::UniquePtr<LayoutManager>(new DiagnosticStackManager(counts)));
      }

      const LayoutRect slot(0.0f, 0.0f, 922.50830078125f, 22.0f);
      auto             pass = [&]()
      {
        parent.Measure(slot.width, slot.height);
        parent.Arrange(slot);
      };
      pass();
      PrintManagerDiagnostic(grid ? "grid" : "stack", "initial", padding * 2.0f, counts, child);
      DALI_TEST_CHECK(counts.measures == 1 && counts.arranges == 1);

      counts.measures = 0;
      counts.arranges = 0;
      Counter(child).Reset();
      for(int i = 0; i < 16; ++i)
      {
        // Only the parent is explicitly dirtied. The child's actual manager
        // constraints may still differ between phases; record rather than hide
        // those misses or pretend the default View fix covers manager wrappers.
        parent.InvalidateMeasure();
        pass();
      }
      PrintManagerDiagnostic(grid ? "grid" : "stack", "parent-dirty-16", padding * 2.0f, counts, child);
      DALI_TEST_CHECK(counts.measures == 16 && counts.arranges == 16);
      DALI_TEST_CHECK(parent.GetProperty<float>(Actor::Property::SIZE_WIDTH) == slot.width);
      DALI_TEST_CHECK(parent.GetProperty<float>(Actor::Property::SIZE_HEIGHT) == slot.height);
    }
  }
  END_TEST;
}
