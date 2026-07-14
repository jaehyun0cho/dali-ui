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
#include <dali-ui-foundation/public-api/layouts/flex-layout-params.h>

// EXTERNAL INCLUDES
#include <algorithm>
#include <new>
#include <type_traits>
#include <utility>

namespace Dali
{
namespace Ui
{

class FlexLayoutParams::Impl
{
public:
  Impl()
  : mFlexGrow(0.0f),
    mFlexShrink(1.0f),
    mFlexBasis(WRAP_CONTENT),
    mAlignSelf(FlexAlign::AUTO)
  {
  }

  float     mFlexGrow;
  float     mFlexShrink;
  float     mFlexBasis;
  FlexAlign mAlignSelf;
};

static_assert(sizeof(FlexLayoutParams) == 24u, "FlexLayoutParams ABI size changed");
static_assert(alignof(FlexLayoutParams) == 8u, "FlexLayoutParams ABI alignment changed");

void FlexLayoutParams::ValidateStorage() noexcept
{
  static_assert(sizeof(Impl) <= STORAGE_SIZE, "FlexLayoutParams storage is too small");
  static_assert(alignof(Impl) <= STORAGE_ALIGNMENT, "FlexLayoutParams storage alignment is insufficient");
  static_assert(std::is_nothrow_move_constructible_v<Impl>, "FlexLayoutParams::Impl move construction must be noexcept");
  static_assert(std::is_nothrow_move_assignable_v<Impl>, "FlexLayoutParams::Impl move assignment must be noexcept");
  static_assert(std::is_nothrow_destructible_v<Impl>, "FlexLayoutParams::Impl destruction must be noexcept");
}

FlexLayoutParams::Impl* FlexLayoutParams::ImplPtr() noexcept
{
  return std::launder(reinterpret_cast<Impl*>(mStorage));
}

const FlexLayoutParams::Impl* FlexLayoutParams::ImplPtr() const noexcept
{
  return std::launder(reinterpret_cast<const Impl*>(mStorage));
}

FlexLayoutParams::FlexLayoutParams()
{
  ValidateStorage();
  ::new(static_cast<void*>(mStorage)) Impl();
}

FlexLayoutParams::FlexLayoutParams(const FlexLayoutParams& other)
{
  ValidateStorage();
  ::new(static_cast<void*>(mStorage)) Impl(*other.ImplPtr());
}

FlexLayoutParams::FlexLayoutParams(FlexLayoutParams&& other) noexcept
{
  ValidateStorage();
  ::new(static_cast<void*>(mStorage)) Impl(std::move(*other.ImplPtr()));
}

FlexLayoutParams& FlexLayoutParams::operator=(const FlexLayoutParams& other)
{
  if(this != &other)
  {
    *ImplPtr() = *other.ImplPtr();
  }
  return *this;
}

FlexLayoutParams& FlexLayoutParams::operator=(FlexLayoutParams&& other) noexcept
{
  if(this != &other)
  {
    *ImplPtr() = std::move(*other.ImplPtr());
  }
  return *this;
}

FlexLayoutParams::~FlexLayoutParams()
{
  ImplPtr()->~Impl();
}

FlexLayoutParams FlexLayoutParams::New()
{
  return FlexLayoutParams();
}

FlexLayoutParams FlexLayoutParams::New(const FlexLayoutParams& other)
{
  return FlexLayoutParams(other);
}

FlexLayoutParams& FlexLayoutParams::SetFlexGrow(float grow)
{
  ImplPtr()->mFlexGrow = std::max(0.0f, grow);
  return *this;
}

float FlexLayoutParams::GetFlexGrow() const
{
  return ImplPtr()->mFlexGrow;
}

FlexLayoutParams& FlexLayoutParams::SetFlexShrink(float shrink)
{
  ImplPtr()->mFlexShrink = std::max(0.0f, shrink);
  return *this;
}

float FlexLayoutParams::GetFlexShrink() const
{
  return ImplPtr()->mFlexShrink;
}

FlexLayoutParams& FlexLayoutParams::SetFlexBasis(float basis)
{
  ImplPtr()->mFlexBasis = basis;
  return *this;
}

float FlexLayoutParams::GetFlexBasis() const
{
  return ImplPtr()->mFlexBasis;
}

FlexLayoutParams& FlexLayoutParams::SetAlignSelf(FlexAlign align)
{
  ImplPtr()->mAlignSelf = align;
  return *this;
}

FlexAlign FlexLayoutParams::GetAlignSelf() const
{
  return ImplPtr()->mAlignSelf;
}

} // namespace Ui
} // namespace Dali
