#pragma once

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

// INTERNAL INCLUDES
#include <dali-ui-foundation/integration-api/reserved-trait-id.h>
#include <dali-ui-foundation/integration-api/view-integ.h>
#include <dali-ui-foundation/public-api/layouts/grid-layout-params.h>
#include <dali-ui-foundation/public-api/traits/trait-id.h>
#include <dali-ui-foundation/public-api/traits/trait-object.h>

namespace Dali
{
namespace Ui
{
namespace Internal
{

/**
 * @brief Trait implementation that stores GridLayout child parameters.
 */
class GridLayoutParamsTrait final : public TraitObject
{
public:
  explicit GridLayoutParamsTrait(const GridLayoutParams& params)
  : mRow(params.GetRow()),
    mColumn(params.GetColumn()),
    mRowSpan(params.GetRowSpan()),
    mColumnSpan(params.GetColumnSpan()),
    mHorizontalAlignment(params.GetHorizontalAlignment()),
    mVerticalAlignment(params.GetVerticalAlignment())
  {
  }

  void CopyTo(GridLayoutParams& params) const
  {
    params.SetRow(mRow)
      .SetColumn(mColumn)
      .SetRowSpan(mRowSpan)
      .SetColumnSpan(mColumnSpan)
      .SetHorizontalAlignment(mHorizontalAlignment)
      .SetVerticalAlignment(mVerticalAlignment);
  }

  /**
   * @brief Gets the row index for this child in the grid.
   * @return The row index.
   */
  uint32_t GetRow() const
  {
    return mRow;
  }

  /**
   * @brief Gets the column index for this child in the grid.
   * @return The column index.
   */
  uint32_t GetColumn() const
  {
    return mColumn;
  }

  /**
   * @brief Gets how many rows this child spans.
   * @return The row span count.
   */
  uint32_t GetRowSpan() const
  {
    return mRowSpan;
  }

  /**
   * @brief Gets how many columns this child spans.
   * @return The column span count.
   */
  uint32_t GetColumnSpan() const
  {
    return mColumnSpan;
  }

  LayoutAlignment GetHorizontalAlignment() const
  {
    return mHorizontalAlignment;
  }

  LayoutAlignment GetVerticalAlignment() const
  {
    return mVerticalAlignment;
  }

  /**
   * @brief Retrieves the GridLayoutParams trait attached to a view, if any.
   * @param[in] viewImpl The view implementation to query.
   * @return Pointer to the params, or nullptr if not attached.
   * @warning Do not retain the pointer across trait replacement or removal.
   */
  static const GridLayoutParamsTrait* Get(const ViewImpl& viewImpl)
  {
    IntrusivePtr<TraitObject> object = Integration::View::GetTrait(viewImpl, Integration::ReservedTraitId::GRID_LAYOUT_PARAMS);
    DALI_ASSERT_DEBUG(!object || (dynamic_cast<const GridLayoutParamsTrait*>(object.Get()) && "GRID_LAYOUT_PARAMS trait must be a GridLayoutParamsTrait"));
    return object ? static_cast<const GridLayoutParamsTrait*>(object.Get()) : nullptr;
  }

protected:
  ~GridLayoutParamsTrait() override = default;

private:
  const uint32_t        mRow;
  const uint32_t        mColumn;
  const uint32_t        mRowSpan;
  const uint32_t        mColumnSpan;
  const LayoutAlignment mHorizontalAlignment;
  const LayoutAlignment mVerticalAlignment;
};

} // namespace Internal

} // namespace Ui
} // namespace Dali
