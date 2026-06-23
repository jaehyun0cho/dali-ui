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
#include <dali-ui-foundation/public-api/selection-group.h>

// INTERNAL INCLUDES
#include <dali-ui-foundation/internal/views/view/selection-group-impl.h>

namespace Dali
{

namespace Ui
{

SelectionGroup::SelectionGroup() = default;

SelectionGroup::~SelectionGroup() = default;

SelectionGroup::SelectionGroup(Internal::SelectionGroupImpl* implementation)
: BaseHandle(implementation)
{
}

SelectionGroup::SelectionGroup(const SelectionGroup& handle) = default;

SelectionGroup::SelectionGroup(SelectionGroup&& rhs) noexcept = default;

SelectionGroup& SelectionGroup::operator=(const SelectionGroup& handle) = default;

SelectionGroup& SelectionGroup::operator=(SelectionGroup&& rhs) noexcept = default;

SelectionGroup SelectionGroup::New()
{
  IntrusivePtr<Internal::SelectionGroupImpl> internal = Internal::SelectionGroupImpl::New();
  return SelectionGroup(internal.Get());
}

SelectionGroup SelectionGroup::DownCast(BaseHandle handle)
{
  return SelectionGroup(dynamic_cast<Internal::SelectionGroupImpl*>(handle.GetObjectPtr()));
}

Signal<void(View, View, InputEvent)>& SelectionGroup::SelectedMemberChangedSignal()
{
  return GetImpl(*this).SelectedMemberChangedSignal();
}

void SelectionGroup::Add(View member)
{
  GetImpl(*this).Add(member);
}

void SelectionGroup::Remove(View member)
{
  GetImpl(*this).Remove(member);
}

uint32_t SelectionGroup::GetMemberCount() const
{
  return GetImpl(*this).GetMemberCount();
}

View SelectionGroup::GetSelectedMember() const
{
  return GetImpl(*this).GetSelectedMember();
}

void SelectionGroup::ClearSelection()
{
  GetImpl(*this).ClearSelection();
}

} // namespace Ui

} // namespace Dali
