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

// EXTERNAL INCLUDES
#include <dali/public-api/object/weak-handle.h>
#include <algorithm>
#include <map>
#include <string>
#include <utility>
#include <vector>

// INTERNAL INCLUDES
#include <dali-ui-foundation/integration-api/selection-group-impl.h>
#include <dali-ui-foundation/public-api/view.h>

namespace Dali
{

namespace Ui
{

namespace
{

/**
 * @brief File-local registry mapping a name to a weakly-held SelectionGroup.
 *
 * Weak so the registry never extends a group's lifetime: once every handle and
 * member is gone the entry becomes dead and is purged on the next lookup.
 */
std::map<std::string, WeakHandle<SelectionGroup>>& NamedRegistry()
{
  static std::map<std::string, WeakHandle<SelectionGroup>> registry;
  return registry;
}

/**
 * @brief File-local registry mapping a parent View to a weakly-held SelectionGroup.
 *
 * A flat vector scanned linearly: no new key type or comparator is introduced. Both
 * the parent View and the group are held weakly. Dead entries (either side gone) are
 * purged on each lookup.
 */
std::vector<std::pair<WeakHandle<View>, WeakHandle<SelectionGroup>>>& ParentRegistry()
{
  static std::vector<std::pair<WeakHandle<View>, WeakHandle<SelectionGroup>>> registry;
  return registry;
}

} // unnamed namespace

SelectionGroup::SelectionGroup() = default;

SelectionGroup SelectionGroup::New()
{
  IntrusivePtr<Integration::SelectionGroupImpl> impl = new Integration::SelectionGroupImpl();
  return SelectionGroup(impl.Get());
}

SelectionGroup SelectionGroup::Find(const std::string& name)
{
  auto& registry = NamedRegistry();

  // Purge dead weak entries so the registry never holds stale names.
  for(auto it = registry.begin(); it != registry.end();)
  {
    if(!it->second.GetHandle())
    {
      it = registry.erase(it);
    }
    else
    {
      ++it;
    }
  }

  auto found = registry.find(name);
  if(found != registry.end())
  {
    SelectionGroup live = found->second.GetHandle();
    if(live)
    {
      return live;
    }
  }

  SelectionGroup group = SelectionGroup::New();
  registry[name]       = WeakHandle<SelectionGroup>(group);
  return group;
}

SelectionGroup SelectionGroup::Find(View parentView)
{
  auto& registry = ParentRegistry();

  // Purge dead weak entries (either the parent View or the group gone).
  registry.erase(
    std::remove_if(registry.begin(),
                   registry.end(),
                   [](const std::pair<WeakHandle<View>, WeakHandle<SelectionGroup>>& entry)
  { return !entry.first.GetHandle() || !entry.second.GetHandle(); }),
    registry.end());

  if(parentView)
  {
    for(const auto& entry : registry)
    {
      if(entry.first.GetHandle() == parentView)
      {
        SelectionGroup live = entry.second.GetHandle();
        if(live)
        {
          return live;
        }
      }
    }
  }

  SelectionGroup group = SelectionGroup::New();
  registry.push_back(std::make_pair(WeakHandle<View>(parentView), WeakHandle<SelectionGroup>(group)));
  return group;
}

SelectionGroup SelectionGroup::DownCast(BaseHandle handle)
{
  return SelectionGroup(dynamic_cast<Integration::SelectionGroupImpl*>(handle.GetObjectPtr()));
}

SelectionGroup::SelectionGroup(const SelectionGroup& selectionGroup) = default;

SelectionGroup::SelectionGroup(SelectionGroup&& rhs) noexcept = default;

SelectionGroup& SelectionGroup::operator=(const SelectionGroup& rhs) = default;

SelectionGroup& SelectionGroup::operator=(SelectionGroup&& rhs) noexcept = default;

SelectionGroup::~SelectionGroup() = default;

SelectionGroup::SelectionGroup(Integration::SelectionGroupImpl* implementation)
: BaseHandle(implementation)
{
}

Signal<void(View, View, InputEvent)>& SelectionGroup::SelectedMemberChangedSignal()
{
  return GetImpl(*this).SelectedMemberChangedSignal();
}

bool SelectionGroup::Add(View member)
{
  return GetImpl(*this).Add(member);
}

bool SelectionGroup::Remove(View member)
{
  return GetImpl(*this).Remove(member);
}

uint32_t SelectionGroup::GetMemberCount() const
{
  return GetImpl(*this).GetMemberCount();
}

View SelectionGroup::GetSelectedMember() const
{
  return GetImpl(*this).GetSelectedMember();
}

bool SelectionGroup::SelectMember(View member)
{
  return GetImpl(*this).SelectMember(member);
}

bool SelectionGroup::ClearSelection()
{
  return GetImpl(*this).ClearSelection();
}

void SelectionGroup::SetAllowEmptySelection(bool allow)
{
  GetImpl(*this).SetAllowEmptySelection(allow);
}

bool SelectionGroup::IsEmptySelectionAllowed() const
{
  return GetImpl(*this).IsEmptySelectionAllowed();
}

} // namespace Ui

} // namespace Dali
