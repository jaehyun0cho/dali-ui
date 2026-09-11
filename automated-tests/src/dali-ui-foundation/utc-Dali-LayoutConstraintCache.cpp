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
#include <dali-ui-test-suite-utils.h>
#include <dali.h>
#include <dali/devel-api/object/type-registry.h>

#include <cstdio>
#include <limits>

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

  int          measures{0};
  int          arranges{0};
  MeasuredSize lastConstraint{0.0f, 0.0f};
  LayoutRect   lastSlot{0.0f, 0.0f, 0.0f, 0.0f};

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
    lastSlot = bounds;
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
