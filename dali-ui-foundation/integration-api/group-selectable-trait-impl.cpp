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

// INTERNAL INCLUDES
#include <dali-ui-foundation/integration-api/selectable-group-impl.h>
#include <dali-ui-foundation/public-api/view-accessibility-enums.h>
#include <dali-ui-foundation/public-api/view-impl.h>
#include <dali-ui-foundation/public-api/view.h>

namespace Dali
{

namespace Ui
{

namespace Integration
{

GroupSelectableTraitImpl::GroupSelectableTraitImpl(SelectableGroupImpl* group)
: SelectableTraitImpl(),
  mGroup(group),
  mPreviousRole(static_cast<int32_t>(AccessibilityRole::NONE))
{
}

GroupSelectableTraitImpl::~GroupSelectableTraitImpl()
{
}

void GroupSelectableTraitImpl::CommitSelectionFromGroup(bool selected, InputEvent event)
{
  // The group has already arbitrated this change; commit it without re-vetoing.
  CommitSelectedState(selected, event);
}

View GroupSelectableTraitImpl::GetOwnerView() const
{
  return GetOwner();
}

void GroupSelectableTraitImpl::BindGroup(SelectableGroupImpl* group)
{
  mGroup = group;
}

void GroupSelectableTraitImpl::UnbindGroup(SelectableGroupImpl* group)
{
  if(mGroup.Get() == group)
  {
    mGroup.Reset();
  }
}

void GroupSelectableTraitImpl::ApplyRadioAccessibilityOnBind()
{
  // Re-apply the radio costume on the rebind/re-Add path. OnAttached does not run
  // again there (the trait stays attached, so re-running it would trip the base's
  // single-owner assert), so the group calls this in place to re-acquire RADIO_BUTTON
  // and re-seed CHECKED. Re-captures mPreviousRole from the role live just before this
  // bind, so a later Remove restores that role rather than a frozen first-attach value.
  ApplyRadioCostume();
}

void GroupSelectableTraitImpl::ResetAccessibilityOnRemove()
{
  View owner = GetOwnerView();
  if(owner)
  {
    // Undo the radio costume: clear CHECKED while the role is still RADIO_BUTTON, then
    // restore the role captured at the last (re)bind. The selected bool is left as-is so
    // the View stays an ordinary selectable rather than becoming a deselected one.
    WriteCheckedState(owner, false);
    owner.SetProperty(Ui::View::Property::ACCESSIBILITY_ROLE, mPreviousRole);
  }
}

SelectableGroupImpl* GroupSelectableTraitImpl::GetGroupImpl() const
{
  return mGroup.Get();
}

SelectableGroup GroupSelectableTraitImpl::GetGroup() const
{
  return mGroup ? SelectableGroup(mGroup.Get()) : SelectableGroup();
}

bool GroupSelectableTraitImpl::OnSelectionChanging(View view, bool newSelected)
{
  if(!mGroup)
  {
    // Not bound to a group (e.g. after Remove): behave like a plain selectable.
    return true;
  }
  return mGroup->RequestSelectionChange(*this, GetOwnerView(), newSelected);
}

void GroupSelectableTraitImpl::OnSelectionChanged(View view, bool selected, InputEvent event)
{
  // Keep the accessibility CHECKED bit in lock-step with the committed state
  // (both winner true and loser false), then let the group flush its signal.
  WriteCheckedState(view, selected);

  if(mGroup)
  {
    mGroup->NotifyCommitted(*this, view, selected, event);
  }
}

void GroupSelectableTraitImpl::OnAttached(TraitId id, View& view)
{
  // Base first: sets mOwner (single-owner assert) and wires up toggle-by-click.
  SelectableTraitImpl::OnAttached(id, view);

  // Apply the radio costume (capture role, set RADIO_BUTTON if default, seed CHECKED).
  // The rebind/re-Add path re-applies this same costume via ApplyRadioAccessibilityOnBind
  // because OnAttached does not run again there. Winner arbitration lives in
  // SelectableGroup::ReconcileNewMember.
  ApplyRadioCostume();
}

void GroupSelectableTraitImpl::OnDetaching(TraitId id, View& view)
{
  // Leave the group before the base clears the owner (a destroy mid-transition
  // must cancel any pending transition this trait initiated).
  if(mGroup)
  {
    mGroup->UnregisterMember(*this);
  }
  SelectableTraitImpl::OnDetaching(id, view);
}

void GroupSelectableTraitImpl::OnViewDestroying(ViewImpl* viewImpl)
{
  if(mGroup)
  {
    mGroup->UnregisterMember(*this);
  }
  SelectableTraitImpl::OnViewDestroying(viewImpl);
}

void GroupSelectableTraitImpl::ApplyRadioCostume()
{
  View owner = GetOwnerView();
  if(!owner)
  {
    return;
  }

  // Capture the host's role so Remove can restore it, then apply RADIO_BUTTON only
  // when the host left the role at its default (NONE). This avoids clobbering a role
  // the application set deliberately. Re-captured on every (re)bind so the restored
  // role tracks the role live just before this bind, not a frozen first-attach value.
  // (An explicit NONE is indistinguishable from the default and is replaced.)
  //
  // Capture-guard: skip the capture when the live role is already our own costume
  // (RADIO_BUTTON). On a cross-group rebind there is no intervening Remove, so the
  // previous group's RADIO_BUTTON is still live; capturing it would overwrite the true
  // original (held in mPreviousRole from first attach or the last Remove reset) and a
  // later Remove would wrongly restore RADIO_BUTTON instead of the original NONE.
  // (An application that deliberately presets RADIO_BUTTON before Add is indistinguishable
  // from our costume and is therefore restored to NONE on Remove; same nature as NONE.)
  const int currentRole = owner.GetProperty<int>(Ui::View::Property::ACCESSIBILITY_ROLE);
  if(currentRole != static_cast<int>(AccessibilityRole::RADIO_BUTTON))
  {
    mPreviousRole = currentRole;
  }
  if(mPreviousRole == static_cast<int>(AccessibilityRole::NONE))
  {
    owner.SetProperty(Ui::View::Property::ACCESSIBILITY_ROLE, AccessibilityRole::RADIO_BUTTON);
  }

  // Seed CHECKED from the committed selected state. On the winner-promotion path the
  // group leaves the selected bool true, so the seeded CHECKED stays set; on the
  // no-steal path the subsequent commit drives CHECKED back to false last.
  WriteCheckedState(owner, IsSelected());
}

void GroupSelectableTraitImpl::WriteCheckedState(View view, bool checked)
{
  if(!view)
  {
    return;
  }

  // ACCESSIBILITY_STATES is a whole-bitset property, so a read-modify-write is
  // required to avoid clearing other bits (e.g. ENABLED).
  const int mask = 1 << static_cast<int>(AccessibilityState::CHECKED);
  int       raw  = view.GetProperty<int>(Ui::View::Property::ACCESSIBILITY_STATES);
  view.SetProperty(Ui::View::Property::ACCESSIBILITY_STATES, checked ? (raw | mask) : (raw & ~mask));
}

} // namespace Integration

} // namespace Ui

} // namespace Dali
