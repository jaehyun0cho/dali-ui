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
#include <dali-ui-foundation/public-api/layouts/absolute-layout-params.h>

// EXTERNAL INCLUDES
#include <new>
#include <type_traits>
#include <utility>

namespace Dali
{
namespace Ui
{

class AbsoluteLayoutParams::Impl
{
public:
  Impl()
  : mBounds(0.0f, 0.0f, -1.0f, -1.0f),
    mFlags(AbsoluteLayoutFlags::NONE)
  {
  }

  LayoutRect          mBounds;
  AbsoluteLayoutFlags mFlags;
};

static_assert(sizeof(AbsoluteLayoutParams) == 32u, "AbsoluteLayoutParams ABI size changed");
static_assert(alignof(AbsoluteLayoutParams) == 8u, "AbsoluteLayoutParams ABI alignment changed");

void AbsoluteLayoutParams::ValidateStorage() noexcept
{
  static_assert(sizeof(Impl) <= STORAGE_SIZE, "AbsoluteLayoutParams storage is too small");
  static_assert(alignof(Impl) <= STORAGE_ALIGNMENT, "AbsoluteLayoutParams storage alignment is insufficient");
  static_assert(std::is_nothrow_move_constructible_v<Impl>, "AbsoluteLayoutParams::Impl move construction must be noexcept");
  static_assert(std::is_nothrow_move_assignable_v<Impl>, "AbsoluteLayoutParams::Impl move assignment must be noexcept");
  static_assert(std::is_nothrow_destructible_v<Impl>, "AbsoluteLayoutParams::Impl destruction must be noexcept");
}

AbsoluteLayoutParams::Impl* AbsoluteLayoutParams::ImplPtr() noexcept
{
  return std::launder(reinterpret_cast<Impl*>(mStorage));
}

const AbsoluteLayoutParams::Impl* AbsoluteLayoutParams::ImplPtr() const noexcept
{
  return std::launder(reinterpret_cast<const Impl*>(mStorage));
}

AbsoluteLayoutParams::AbsoluteLayoutParams()
{
  ValidateStorage();
  ::new(static_cast<void*>(mStorage)) Impl();
}

AbsoluteLayoutParams::AbsoluteLayoutParams(const AbsoluteLayoutParams& other)
{
  ValidateStorage();
  ::new(static_cast<void*>(mStorage)) Impl(*other.ImplPtr());
}

AbsoluteLayoutParams::AbsoluteLayoutParams(AbsoluteLayoutParams&& other) noexcept
{
  ValidateStorage();
  ::new(static_cast<void*>(mStorage)) Impl(std::move(*other.ImplPtr()));
}

AbsoluteLayoutParams& AbsoluteLayoutParams::operator=(const AbsoluteLayoutParams& other)
{
  if(this != &other)
  {
    *ImplPtr() = *other.ImplPtr();
  }
  return *this;
}

AbsoluteLayoutParams& AbsoluteLayoutParams::operator=(AbsoluteLayoutParams&& other) noexcept
{
  if(this != &other)
  {
    *ImplPtr() = std::move(*other.ImplPtr());
  }
  return *this;
}

AbsoluteLayoutParams::~AbsoluteLayoutParams()
{
  ImplPtr()->~Impl();
}

AbsoluteLayoutParams AbsoluteLayoutParams::New()
{
  return AbsoluteLayoutParams();
}

AbsoluteLayoutParams AbsoluteLayoutParams::New(const AbsoluteLayoutParams& other)
{
  return AbsoluteLayoutParams(other);
}

AbsoluteLayoutParams& AbsoluteLayoutParams::SetBounds(const LayoutRect& bounds)
{
  ImplPtr()->mBounds = bounds;
  return *this;
}

LayoutRect AbsoluteLayoutParams::GetBounds() const
{
  return ImplPtr()->mBounds;
}

AbsoluteLayoutParams& AbsoluteLayoutParams::SetX(float x)
{
  ImplPtr()->mBounds.SetX(x);
  return *this;
}

float AbsoluteLayoutParams::GetX() const
{
  return ImplPtr()->mBounds.GetX();
}

AbsoluteLayoutParams& AbsoluteLayoutParams::SetY(float y)
{
  ImplPtr()->mBounds.SetY(y);
  return *this;
}

float AbsoluteLayoutParams::GetY() const
{
  return ImplPtr()->mBounds.GetY();
}

AbsoluteLayoutParams& AbsoluteLayoutParams::SetWidth(float width)
{
  ImplPtr()->mBounds.SetWidth(width);
  return *this;
}

float AbsoluteLayoutParams::GetWidth() const
{
  return ImplPtr()->mBounds.GetWidth();
}

AbsoluteLayoutParams& AbsoluteLayoutParams::SetHeight(float height)
{
  ImplPtr()->mBounds.SetHeight(height);
  return *this;
}

float AbsoluteLayoutParams::GetHeight() const
{
  return ImplPtr()->mBounds.GetHeight();
}

AbsoluteLayoutParams& AbsoluteLayoutParams::SetFlags(AbsoluteLayoutFlags flags)
{
  ImplPtr()->mFlags = flags;
  return *this;
}

AbsoluteLayoutFlags AbsoluteLayoutParams::GetFlags() const
{
  return ImplPtr()->mFlags;
}

} // namespace Ui
} // namespace Dali
