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
#include <dali-ui-foundation/integration-api/group-selectable-trait-impl.h>

// EXTERNAL INCLUDES
#include <dali/public-api/actors/actor.h>
#include <dali/public-api/common/dali-common.h>

// INTERNAL INCLUDES
#include <dali-ui-foundation/public-api/selectable-trait.h>
#include <dali-ui-foundation/public-api/view.h>

namespace Dali
{

namespace Ui
{

namespace Integration
{

namespace
{

/**
 * @brief Resolves the parent View of the given View, or an empty handle.
 */
View GetParentView(View view)
{
  if(!view)
  {
    return View();
  }
  return View::DownCast(view.GetParent());
}

} // unnamed namespace

GroupSelectableTraitImpl::GroupSelectableTraitImpl()
: mOwner(),
  mGroupName(),
  mAutoGroupParent(),
  mAutoGroupByParent(true)
{
}

GroupSelectableTraitImpl::~GroupSelectableTraitImpl()
{
}

void GroupSelectableTraitImpl::OnAttached(TraitId /*id*/, View& view)
{
  DALI_ASSERT_ALWAYS(!(mOwner.GetHandle()) && "The trait can not be attached multiple target views");
  mOwner = view;

  // Compose the View's SelectableTrait: ensure one exists (reuse if already present,
  // never swap). This makes the View selectable so it can participate in the group's
  // mutual exclusion. The arbitration itself lives in the SelectableTrait.
  view.AsSelectable();

  view.OnSceneSignal().Connect(this, &GroupSelectableTraitImpl::OnOwnerOnScene);
  view.OffSceneSignal().Connect(this, &GroupSelectableTraitImpl::OnOwnerOffScene);
}

void GroupSelectableTraitImpl::OnDetaching(TraitId /*id*/, View& /*view*/)
{
  DisconnectAll();

  LeaveCurrentGroup();

  if(!mGroupName.empty())
  {
    View owner = mOwner.GetHandle();
    if(owner)
    {
      SelectionGroup::Find(mGroupName).Remove(owner);
    }
  }

  mGroupName.clear();
  mOwner.Reset();
}

void GroupSelectableTraitImpl::OnViewDestroying(ViewImpl* /*viewImpl*/)
{
  DisconnectAll();

  // The owner View is being destroyed; its SelectableTrait detaches itself from any
  // group on its own OnViewDestroying. Nothing strong is held here, so just drop the
  // weak records.
  mAutoGroupParent.Reset();
  mOwner.Reset();
}

void GroupSelectableTraitImpl::OnOwnerOnScene(Dali::Actor /*actor*/)
{
  // Explicit name wins: SetGroupName already joined the named group eagerly.
  if(!mGroupName.empty())
  {
    return;
  }

  JoinParentGroupIfApplicable();
}

void GroupSelectableTraitImpl::OnOwnerOffScene(Dali::Actor /*actor*/)
{
  // Parent-auto membership is scene-scoped: leave it on disconnect. Explicit-named
  // membership is NOT touched here (it persists across scene connection cycles).
  View owner  = mOwner.GetHandle();
  View parent = mAutoGroupParent.GetHandle();
  if(owner && parent)
  {
    SelectionGroup::Find(parent).Remove(owner);
  }
  mAutoGroupParent.Reset();
}

void GroupSelectableTraitImpl::SetGroupName(const std::string& name)
{
  if(name == mGroupName)
  {
    return;
  }

  View owner = mOwner.GetHandle();

  // Leave the old explicit-named group, if any.
  if(!mGroupName.empty() && owner)
  {
    SelectionGroup::Find(mGroupName).Remove(owner);
  }

  // If currently auto-joined a parent group, leave it too: an explicit name takes
  // precedence over parent auto-grouping.
  LeaveCurrentGroup();

  if(!name.empty() && owner)
  {
    SelectionGroup::Find(name).Add(owner);
  }

  mGroupName = name;

  // Clearing the name on-scene: parent auto-grouping takes over immediately rather
  // than waiting for the next scene reconnection. When off-scene, do nothing now;
  // mGroupName is already empty, so the next OnOwnerOnScene will auto-join
  // (preserving the scene-scoped semantics of auto-membership).
  if(name.empty() && owner && owner.GetProperty<bool>(Dali::Actor::Property::CONNECTED_TO_SCENE))
  {
    JoinParentGroupIfApplicable();
  }
}

std::string GroupSelectableTraitImpl::GetGroupName() const
{
  return mGroupName;
}

SelectionGroup GroupSelectableTraitImpl::GetGroup() const
{
  if(!mGroupName.empty())
  {
    return SelectionGroup::Find(mGroupName);
  }

  // No explicit name: report whatever group the View's SelectableTrait is bound to.
  // For a parent-auto-grouped member this is its parent's group; otherwise empty.
  View owner = mOwner.GetHandle();
  if(owner)
  {
    return owner.AsSelectable().GetGroup();
  }
  return SelectionGroup();
}

void GroupSelectableTraitImpl::LeaveCurrentGroup()
{
  View owner  = mOwner.GetHandle();
  View parent = mAutoGroupParent.GetHandle();
  if(owner && parent)
  {
    SelectionGroup::Find(parent).Remove(owner);
  }
  mAutoGroupParent.Reset();
}

void GroupSelectableTraitImpl::JoinParentGroupIfApplicable()
{
  if(!mAutoGroupByParent)
  {
    return;
  }

  View owner = mOwner.GetHandle();
  if(!owner)
  {
    return;
  }

  View parent = GetParentView(owner);
  if(!parent)
  {
    return;
  }

  // Idempotent: if already auto-joined this same parent, nothing to do.
  if(mAutoGroupParent.GetHandle() == parent)
  {
    return;
  }

  // Leave a previously auto-joined parent group (e.g. after reparenting before a
  // disconnect) before joining the new parent's group.
  View previousParent = mAutoGroupParent.GetHandle();
  if(previousParent)
  {
    SelectionGroup::Find(previousParent).Remove(owner);
  }

  SelectionGroup::Find(parent).Add(owner);
  mAutoGroupParent = parent;
}

} // namespace Integration

} // namespace Ui

} // namespace Dali
