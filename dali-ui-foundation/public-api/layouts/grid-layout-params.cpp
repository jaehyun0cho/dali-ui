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

// CLASS HEADER
#include <dali-ui-foundation/public-api/layouts/grid-layout-params.h>

// EXTERNAL INCLUDES
#include <algorithm>
#include <new>
#include <type_traits>
#include <utility>

namespace Dali
{
namespace Ui
{

class GridLayoutParams::Impl
{
public:
  Impl()
  : mRow(0u),
    mColumn(0u),
    mRowSpan(1u),
    mColumnSpan(1u),
    mHorizontalAlignment(LayoutAlignment::FILL),
    mVerticalAlignment(LayoutAlignment::FILL)
  {
  }

  uint32_t        mRow;
  uint32_t        mColumn;
  uint32_t        mRowSpan;
  uint32_t        mColumnSpan;
  LayoutAlignment mHorizontalAlignment;
  LayoutAlignment mVerticalAlignment;
};

static_assert(sizeof(GridLayoutParams) == 32u, "GridLayoutParams ABI size changed");
static_assert(alignof(GridLayoutParams) == 8u, "GridLayoutParams ABI alignment changed");

void GridLayoutParams::ValidateStorage() noexcept
{
  static_assert(sizeof(Impl) <= STORAGE_SIZE, "GridLayoutParams storage is too small");
  static_assert(alignof(Impl) <= STORAGE_ALIGNMENT, "GridLayoutParams storage alignment is insufficient");
  static_assert(std::is_nothrow_move_constructible_v<Impl>, "GridLayoutParams::Impl move construction must be noexcept");
  static_assert(std::is_nothrow_move_assignable_v<Impl>, "GridLayoutParams::Impl move assignment must be noexcept");
  static_assert(std::is_nothrow_destructible_v<Impl>, "GridLayoutParams::Impl destruction must be noexcept");
}

GridLayoutParams::Impl* GridLayoutParams::ImplPtr() noexcept
{
  return std::launder(reinterpret_cast<Impl*>(mStorage));
}

const GridLayoutParams::Impl* GridLayoutParams::ImplPtr() const noexcept
{
  return std::launder(reinterpret_cast<const Impl*>(mStorage));
}

GridLayoutParams::GridLayoutParams()
{
  ValidateStorage();
  ::new(static_cast<void*>(mStorage)) Impl();
}

GridLayoutParams::GridLayoutParams(const GridLayoutParams& other)
{
  ValidateStorage();
  ::new(static_cast<void*>(mStorage)) Impl(*other.ImplPtr());
}

GridLayoutParams::GridLayoutParams(GridLayoutParams&& other) noexcept
{
  ValidateStorage();
  ::new(static_cast<void*>(mStorage)) Impl(std::move(*other.ImplPtr()));
}

GridLayoutParams& GridLayoutParams::operator=(const GridLayoutParams& other)
{
  if(this != &other)
  {
    *ImplPtr() = *other.ImplPtr();
  }
  return *this;
}

GridLayoutParams& GridLayoutParams::operator=(GridLayoutParams&& other) noexcept
{
  if(this != &other)
  {
    *ImplPtr() = std::move(*other.ImplPtr());
  }
  return *this;
}

GridLayoutParams::~GridLayoutParams()
{
  ImplPtr()->~Impl();
}

GridLayoutParams GridLayoutParams::New()
{
  return GridLayoutParams();
}

GridLayoutParams GridLayoutParams::New(const GridLayoutParams& other)
{
  return GridLayoutParams(other);
}

GridLayoutParams& GridLayoutParams::SetRow(uint32_t row)
{
  ImplPtr()->mRow = row;
  return *this;
}

uint32_t GridLayoutParams::GetRow() const
{
  return ImplPtr()->mRow;
}

GridLayoutParams& GridLayoutParams::SetColumn(uint32_t column)
{
  ImplPtr()->mColumn = column;
  return *this;
}

uint32_t GridLayoutParams::GetColumn() const
{
  return ImplPtr()->mColumn;
}

GridLayoutParams& GridLayoutParams::SetRowSpan(uint32_t span)
{
  ImplPtr()->mRowSpan = std::max(1u, span);
  return *this;
}

uint32_t GridLayoutParams::GetRowSpan() const
{
  return ImplPtr()->mRowSpan;
}

GridLayoutParams& GridLayoutParams::SetColumnSpan(uint32_t span)
{
  ImplPtr()->mColumnSpan = std::max(1u, span);
  return *this;
}

uint32_t GridLayoutParams::GetColumnSpan() const
{
  return ImplPtr()->mColumnSpan;
}

GridLayoutParams& GridLayoutParams::SetHorizontalAlignment(LayoutAlignment alignment)
{
  ImplPtr()->mHorizontalAlignment = alignment;
  return *this;
}

LayoutAlignment GridLayoutParams::GetHorizontalAlignment() const
{
  return ImplPtr()->mHorizontalAlignment;
}

GridLayoutParams& GridLayoutParams::SetVerticalAlignment(LayoutAlignment alignment)
{
  ImplPtr()->mVerticalAlignment = alignment;
  return *this;
}

LayoutAlignment GridLayoutParams::GetVerticalAlignment() const
{
  return ImplPtr()->mVerticalAlignment;
}

} // namespace Ui
} // namespace Dali
