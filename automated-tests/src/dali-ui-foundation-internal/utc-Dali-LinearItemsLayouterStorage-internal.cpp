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
 *
 */

#include <algorithm>
#include <limits>

#include <dali-ui-foundation/internal/linear-items-layouter-impl.h>
#include <dali-ui-test-suite-utils.h>

using namespace Dali;
using namespace Dali::Ui;

namespace DALI_NAMESPACE
{
namespace Ui
{
namespace Internal
{
// Inspect the actual vector, so capacity changes and copied live prefixes can
// be checked without allocating a View for every measured item.
class LinearItemsLayouterTestAccessor
{
public:
  static void SetItemCount(LinearItemsLayouterImpl& layouter, uint32_t count)
  {
    layouter.mCachedItemCount = count;
  }

  static void Measure(LinearItemsLayouterImpl& layouter, uint32_t position, float extent)
  {
    layouter.MeasureUpdate(position, extent);
  }

  static const Dali::Vector<float>& Extents(const LinearItemsLayouterImpl& layouter)
  {
    return layouter.mExtentCache;
  }

  static uint32_t MeasuredCount(const LinearItemsLayouterImpl& layouter)
  {
    return layouter.mMeasuredItemCount;
  }

  static float MeasuredTotal(const LinearItemsLayouterImpl& layouter)
  {
    return layouter.mTotalMeasuredExtent;
  }
};
} // namespace Internal
} // namespace Ui
} // namespace DALI_NAMESPACE

using Layouter = Dali::Ui::Internal::LinearItemsLayouterImpl;
using Access   = Dali::Ui::Internal::LinearItemsLayouterTestAccessor;
using SizeType = Dali::Vector<float>::SizeType;

void utc_dali_linear_items_layouter_storage_internal_startup(void)
{
  test_return_value = TET_UNDEF;
}

void utc_dali_linear_items_layouter_storage_internal_cleanup(void)
{
  test_return_value = TET_PASS;
}

int UtcDaliLinearItemsLayouterStorageSequentialGrowthP(void)
{
  UiTestApplication  application;
  auto               layouter  = Layouter::New(ItemsLayouter::Orientation::VERTICAL);
  constexpr uint32_t itemCount = 4096u;
  Access::SetItemCount(*layouter, itemCount);

  uint32_t growthCount   = 0u;
  uint64_t copiedExtents = 0u;
  float    expectedTotal = 0.0f;
  for(uint32_t i = 0u; i < itemCount; ++i)
  {
    const SizeType oldCapacity = Access::Extents(*layouter).Capacity();
    const SizeType oldSize     = Access::Extents(*layouter).Size();
    const float    extent      = 1.0f + static_cast<float>(i % 7u) * 0.25f;
    Access::Measure(*layouter, i, extent);
    expectedTotal += extent;

    const auto& extents = Access::Extents(*layouter);
    if(extents.Capacity() != oldCapacity)
    {
      ++growthCount;
      copiedExtents += oldSize;
    }
    DALI_TEST_EQUALS(extents.Size(), static_cast<SizeType>(i) + 1u, TEST_LOCATION);
    DALI_TEST_CHECK(extents.Capacity() >= extents.Size());
    DALI_TEST_CHECK(extents.Capacity() <= itemCount);
    DALI_TEST_CHECK(extents.Capacity() <= extents.Size() + std::max<SizeType>(1u, extents.Size() / 2u));
  }

  // Bound the live prefixes copied by the real vector to catch repeated
  // exact-size growth without tying the test to individual capacities.
  DALI_TEST_CHECK(growthCount < 32u);
  DALI_TEST_CHECK(copiedExtents < 4u * static_cast<uint64_t>(itemCount));
  DALI_TEST_EQUALS(Access::MeasuredCount(*layouter), itemCount, TEST_LOCATION);
  DALI_TEST_EQUALS(Access::MeasuredTotal(*layouter), expectedTotal, TEST_LOCATION);
  for(uint32_t i = 0u; i < itemCount; ++i)
  {
    DALI_TEST_EQUALS(Access::Extents(*layouter)[i], 1.0f + static_cast<float>(i % 7u) * 0.25f, TEST_LOCATION);
  }
  END_TEST;
}

int UtcDaliLinearItemsLayouterStorageSparseAndRemeasureP(void)
{
  UiTestApplication application;
  auto              layouter = Layouter::New(ItemsLayouter::Orientation::VERTICAL);
  Access::SetItemCount(*layouter, 10000u);
  Access::Measure(*layouter, 4095u, 2.5f);

  const auto& extents = Access::Extents(*layouter);
  DALI_TEST_EQUALS(extents.Size(), SizeType(4096u), TEST_LOCATION);
  DALI_TEST_EQUALS(extents.Capacity(), SizeType(4096u), TEST_LOCATION);
  for(uint32_t i = 0u; i < 4095u; ++i)
  {
    DALI_TEST_EQUALS(extents[i], 0.0f, TEST_LOCATION);
  }
  DALI_TEST_EQUALS(extents[4095u], 2.5f, TEST_LOCATION);
  DALI_TEST_EQUALS(Access::MeasuredCount(*layouter), 1u, TEST_LOCATION);

  Access::Measure(*layouter, 4095u, 3.75f);
  Access::Measure(*layouter, 3000u, 1.5f);
  Access::Measure(*layouter, 12u, 0.25f);
  DALI_TEST_EQUALS(extents.Capacity(), SizeType(4096u), TEST_LOCATION);
  DALI_TEST_EQUALS(extents[4095u], 3.75f, TEST_LOCATION);
  DALI_TEST_EQUALS(extents[3000u], 1.5f, TEST_LOCATION);
  DALI_TEST_EQUALS(extents[12u], 1.0f, TEST_LOCATION);
  DALI_TEST_EQUALS(Access::MeasuredCount(*layouter), 3u, TEST_LOCATION);
  DALI_TEST_EQUALS(Access::MeasuredTotal(*layouter), 6.25f, TEST_LOCATION);
  END_TEST;
}

int UtcDaliLinearItemsLayouterStorageResetRetainsCapacityP(void)
{
  UiTestApplication application;
  auto              layouter = Layouter::New(ItemsLayouter::Orientation::VERTICAL);
  Access::SetItemCount(*layouter, 100u);
  Access::Measure(*layouter, 63u, 5.0f);
  const SizeType capacity = Access::Extents(*layouter).Capacity();

  layouter->OnAdapterChanged();
  DALI_TEST_EQUALS(Access::Extents(*layouter).Size(), SizeType(0u), TEST_LOCATION);
  DALI_TEST_EQUALS(Access::Extents(*layouter).Capacity(), capacity, TEST_LOCATION);
  DALI_TEST_EQUALS(Access::MeasuredCount(*layouter), 0u, TEST_LOCATION);
  DALI_TEST_EQUALS(Access::MeasuredTotal(*layouter), 0.0f, TEST_LOCATION);

  // A smaller replacement adapter reuses retained storage without shrinking
  // it, and old measurements must not survive the clear.
  Access::SetItemCount(*layouter, 3u);
  Access::Measure(*layouter, 2u, 7.0f);
  DALI_TEST_EQUALS(Access::Extents(*layouter).Capacity(), capacity, TEST_LOCATION);
  DALI_TEST_EQUALS(Access::Extents(*layouter)[0u], 0.0f, TEST_LOCATION);
  DALI_TEST_EQUALS(Access::Extents(*layouter)[1u], 0.0f, TEST_LOCATION);
  DALI_TEST_EQUALS(Access::MeasuredCount(*layouter), 1u, TEST_LOCATION);
  DALI_TEST_EQUALS(Access::MeasuredTotal(*layouter), 7.0f, TEST_LOCATION);

  layouter->OnDecorationChanged();
  DALI_TEST_EQUALS(Access::Extents(*layouter).Size(), SizeType(0u), TEST_LOCATION);
  DALI_TEST_EQUALS(Access::Extents(*layouter).Capacity(), capacity, TEST_LOCATION);
  Access::Measure(*layouter, 0u, 2.0f);
  DALI_TEST_EQUALS(Access::Extents(*layouter).Capacity(), capacity, TEST_LOCATION);
  DALI_TEST_EQUALS(Access::MeasuredCount(*layouter), 1u, TEST_LOCATION);
  DALI_TEST_EQUALS(Access::MeasuredTotal(*layouter), 2.0f, TEST_LOCATION);
  END_TEST;
}

int UtcDaliLinearItemsLayouterStorageRejectsOverflowN(void)
{
  UiTestApplication  application;
  auto               layouter        = Layouter::New(ItemsLayouter::Orientation::VERTICAL);
  constexpr SizeType allocationLimit = (std::numeric_limits<SizeType>::max() - 2u * sizeof(SizeType)) / sizeof(float);
  constexpr SizeType itemCountLimit  = std::numeric_limits<uint32_t>::max();
  constexpr uint32_t invalidPosition = static_cast<uint32_t>(std::min(allocationLimit, itemCountLimit));

  // On 32-bit platforms this reaches the allocation-header limit; on 64-bit
  // platforms it rejects the position beyond the largest representable count.
  DALI_TEST_ASSERTION(Access::Measure(*layouter, invalidPosition, 1.0f), "positionIndex < maximumCount");
  TestApplication::EnableLogging(true);
  DALI_TEST_EQUALS(Access::Extents(*layouter).Size(), SizeType(0u), TEST_LOCATION);
  DALI_TEST_EQUALS(Access::Extents(*layouter).Capacity(), SizeType(0u), TEST_LOCATION);
  DALI_TEST_EQUALS(Access::MeasuredCount(*layouter), 0u, TEST_LOCATION);
  END_TEST;
}
