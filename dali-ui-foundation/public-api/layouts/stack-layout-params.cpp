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
#include <dali-ui-foundation/public-api/layouts/stack-layout-params.h>

// EXTERNAL INCLUDES
#include <new>
#include <type_traits>
#include <utility>

namespace Dali
{
namespace Ui
{

class StackLayoutParams::Impl
{
public:
  Impl()
  : mWeight(0.0f),
    mAlignment(LayoutAlignment::START)
  {
  }

  float           mWeight;
  LayoutAlignment mAlignment;
};

static_assert(sizeof(StackLayoutParams) == 16u, "StackLayoutParams ABI size changed");
static_assert(alignof(StackLayoutParams) == 8u, "StackLayoutParams ABI alignment changed");

void StackLayoutParams::ValidateStorage() noexcept
{
  static_assert(sizeof(Impl) <= STORAGE_SIZE, "StackLayoutParams storage is too small");
  static_assert(alignof(Impl) <= STORAGE_ALIGNMENT, "StackLayoutParams storage alignment is insufficient");
  static_assert(std::is_nothrow_move_constructible_v<Impl>, "StackLayoutParams::Impl move construction must be noexcept");
  static_assert(std::is_nothrow_move_assignable_v<Impl>, "StackLayoutParams::Impl move assignment must be noexcept");
  static_assert(std::is_nothrow_destructible_v<Impl>, "StackLayoutParams::Impl destruction must be noexcept");
}

StackLayoutParams::Impl* StackLayoutParams::ImplPtr() noexcept
{
  return std::launder(reinterpret_cast<Impl*>(mStorage));
}

const StackLayoutParams::Impl* StackLayoutParams::ImplPtr() const noexcept
{
  return std::launder(reinterpret_cast<const Impl*>(mStorage));
}

StackLayoutParams::StackLayoutParams()
{
  ValidateStorage();
  ::new(static_cast<void*>(mStorage)) Impl();
}

StackLayoutParams::StackLayoutParams(const StackLayoutParams& other)
{
  ValidateStorage();
  ::new(static_cast<void*>(mStorage)) Impl(*other.ImplPtr());
}

StackLayoutParams::StackLayoutParams(StackLayoutParams&& other) noexcept
{
  ValidateStorage();
  ::new(static_cast<void*>(mStorage)) Impl(std::move(*other.ImplPtr()));
}

StackLayoutParams& StackLayoutParams::operator=(const StackLayoutParams& other)
{
  if(this != &other)
  {
    *ImplPtr() = *other.ImplPtr();
  }
  return *this;
}

StackLayoutParams& StackLayoutParams::operator=(StackLayoutParams&& other) noexcept
{
  if(this != &other)
  {
    *ImplPtr() = std::move(*other.ImplPtr());
  }
  return *this;
}

StackLayoutParams::~StackLayoutParams()
{
  ImplPtr()->~Impl();
}

StackLayoutParams StackLayoutParams::New()
{
  return StackLayoutParams();
}

StackLayoutParams StackLayoutParams::New(const StackLayoutParams& other)
{
  return StackLayoutParams(other);
}

StackLayoutParams& StackLayoutParams::SetWeight(float weight)
{
  ImplPtr()->mWeight = weight;
  return *this;
}

float StackLayoutParams::GetWeight() const
{
  return ImplPtr()->mWeight;
}

StackLayoutParams& StackLayoutParams::SetAlignment(LayoutAlignment alignment)
{
  ImplPtr()->mAlignment = alignment;
  return *this;
}

LayoutAlignment StackLayoutParams::GetAlignment() const
{
  return ImplPtr()->mAlignment;
}

} // namespace Ui
} // namespace Dali
