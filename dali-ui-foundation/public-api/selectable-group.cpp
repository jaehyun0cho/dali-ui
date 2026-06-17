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
#include <dali-ui-foundation/public-api/selectable-group.h>

// INTERNAL INCLUDES
#include <dali-ui-foundation/integration-api/selectable-group-impl.h>
#include <dali-ui-foundation/public-api/view.h>

namespace Dali
{

namespace Ui
{

SelectableGroup::SelectableGroup() = default;

SelectableGroup SelectableGroup::New()
{
  IntrusivePtr<Integration::SelectableGroupImpl> impl = new Integration::SelectableGroupImpl();
  return SelectableGroup(impl.Get());
}

SelectableGroup SelectableGroup::DownCast(BaseHandle handle)
{
  return SelectableGroup(dynamic_cast<Integration::SelectableGroupImpl*>(handle.GetObjectPtr()));
}

SelectableGroup::SelectableGroup(const SelectableGroup& selectableGroup) = default;

SelectableGroup::SelectableGroup(SelectableGroup&& rhs) noexcept = default;

SelectableGroup& SelectableGroup::operator=(const SelectableGroup& rhs) = default;

SelectableGroup& SelectableGroup::operator=(SelectableGroup&& rhs) noexcept = default;

SelectableGroup::~SelectableGroup() = default;

SelectableGroup::SelectableGroup(Integration::SelectableGroupImpl* implementation)
: BaseHandle(implementation)
{
}

Signal<void(View, View, InputEvent)>& SelectableGroup::SelectedMemberChangedSignal()
{
  return GetImpl(*this).SelectedMemberChangedSignal();
}

bool SelectableGroup::Add(View member)
{
  return GetImpl(*this).Add(member);
}

bool SelectableGroup::Remove(View member)
{
  return GetImpl(*this).Remove(member);
}

uint32_t SelectableGroup::GetMemberCount() const
{
  return GetImpl(*this).GetMemberCount();
}

View SelectableGroup::GetSelectedMember() const
{
  return GetImpl(*this).GetSelectedMember();
}

bool SelectableGroup::SelectMember(View member)
{
  return GetImpl(*this).SelectMember(member);
}

bool SelectableGroup::ClearSelection()
{
  return GetImpl(*this).ClearSelection();
}

void SelectableGroup::SetAllowEmptySelection(bool allow)
{
  GetImpl(*this).SetAllowEmptySelection(allow);
}

bool SelectableGroup::IsEmptySelectionAllowed() const
{
  return GetImpl(*this).IsEmptySelectionAllowed();
}

} // namespace Ui

} // namespace Dali
