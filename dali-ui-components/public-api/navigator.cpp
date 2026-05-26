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

// EXTERNAL INCLUDES
#include <dali/devel-api/object/type-registry.h>
#include <utility>

// INTERNAL INCLUDES
#include <dali-ui-components/internal/navigator-impl.h>
#include <dali-ui-components/public-api/navigator.h>

namespace Dali
{

namespace Ui
{

Navigator::Navigator()
{
}

Navigator Navigator::New()
{
  return Internal::NavigatorImpl::New();
}

Navigator::Navigator(const Navigator& navigator)
: View(navigator)
{
}

Navigator::Navigator(Navigator&& rhs) noexcept
: View(std::move(rhs))
{
}

Navigator::~Navigator()
{
}

Navigator& Navigator::operator=(const Navigator& handle)
{
  if(&handle != this)
  {
    Ui::View::operator=(handle);
  }
  return *this;
}

Navigator& Navigator::operator=(Navigator&& rhs) noexcept
{
  Ui::View::operator=(std::move(rhs));
  return *this;
}

Navigator Navigator::DownCast(BaseHandle handle)
{
  Navigator result;
  Ui::View  control = Ui::View::DownCast(handle);
  if(control)
  {
    CustomActorImpl&         customImpl = control.GetImplementation();
    Internal::NavigatorImpl* impl       = dynamic_cast<Internal::NavigatorImpl*>(&customImpl);
    if(impl)
    {
      result = Navigator(customImpl.GetOwner());
    }
  }
  return result;
}

uint32_t Navigator::GetNavigationStackCount() const
{
  return GetImpl(*this).GetNavigationStackCount();
}

uint32_t Navigator::GetModalStackCount() const
{
  return GetImpl(*this).GetModalStackCount();
}

View Navigator::GetNavigationStackAt(uint32_t index) const
{
  return GetImpl(*this).GetNavigationStackAt(index);
}

View Navigator::GetModalStackAt(uint32_t index) const
{
  return GetImpl(*this).GetModalStackAt(index);
}

View Navigator::GetCurrentView() const
{
  return GetImpl(*this).GetCurrentView();
}

void Navigator::Push(View view, bool animated)
{
  GetImpl(*this).Push(view, animated);
}

void Navigator::PushModal(View view, bool animated)
{
  GetImpl(*this).PushModal(view, animated);
}

View Navigator::Pop(bool animated)
{
  return GetImpl(*this).Pop(animated);
}

View Navigator::PopModal(bool animated)
{
  return GetImpl(*this).PopModal(animated);
}

void Navigator::InsertBefore(View view, View before)
{
  GetImpl(*this).InsertBefore(view, before);
}

void Navigator::Remove(View view)
{
  GetImpl(*this).Remove(view);
}

void Navigator::Clear()
{
  GetImpl(*this).Clear();
}

bool Navigator::NavigateBack()
{
  return GetImpl(*this).NavigateBack();
}

Navigator::Navigator(Internal::NavigatorImpl& implementation)
: View(implementation)
{
}

Navigator::Navigator(Dali::Internal::CustomActor* internal)
: View(internal)
{
  VerifyCustomActorPointer<Internal::NavigatorImpl>(internal);
}

} // namespace Ui

} // namespace Dali
