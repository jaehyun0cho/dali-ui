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
#include <dali-ui-foundation/integration-api/selectable-trait-impl.h>

// INTERNAL INCLUDES
#include <dali-ui-foundation/integration-api/reserved-trait-id.h>
#include <dali-ui-foundation/integration-api/selection-group-impl.h>
#include <dali-ui-foundation/integration-api/view-integ.h>
#include <dali-ui-foundation/public-api/input-event.h>
#include <dali-ui-foundation/public-api/interactive-trait.h>
#include <dali-ui-foundation/public-api/view-accessibility-enums.h>
#include <dali-ui-foundation/public-api/view-impl.h>
#include <dali-ui-foundation/public-api/view.h>

namespace Dali::Ui::Integration
{
namespace
{

InteractiveTrait GetInteractiveTrait(ViewImpl& viewImpl)
{
  IntrusivePtr<TraitObject> object     = IntegrationView::GetTrait(viewImpl, ReservedTraitId::INTERACTION_TRAIT);
  auto*                     baseObject = dynamic_cast<BaseObject*>(object.Get());
  return baseObject ? InteractiveTrait::DownCast(BaseHandle(baseObject)) : InteractiveTrait();
}

} // unnamed namespace

SelectableTraitImpl::SelectableTraitImpl()
: mSelectionChangedSignal(),
  mGroup(nullptr),
  mPreviousRole(static_cast<int32_t>(AccessibilityRole::NONE)),
  mSelected(false),
  mToggleByClickEnabled(true),
  mAttached(false)
{
}

SelectableTraitImpl::~SelectableTraitImpl()
{
}

Signal<void(View, bool, InputEvent)>& SelectableTraitImpl::SelectionChangedSignal()
{
  return mSelectionChangedSignal;
}

bool SelectableTraitImpl::IsSelected() const
{
  return mSelected;
}

void SelectableTraitImpl::SetSelected(bool selected)
{
  SetSelectedInternal(selected, InputEvent::Programmatic());
}

void SelectableTraitImpl::SetSelectedInternal(bool selected, InputEvent event)
{
  if(mSelected == selected)
  {
    return;
  }

  View owner = mOwner.GetHandle();
  if(!owner)
  {
    // Not attached yet, just store the state
    mSelected = selected;
    return;
  }

  if(!OnSelectionChanging(owner, selected))
  {
    return;
  }

  CommitSelectedState(selected, event);
}

void SelectableTraitImpl::CommitSelectedState(bool selected, InputEvent event)
{
  if(mSelected == selected)
  {
    return;
  }

  View owner = mOwner.GetHandle();
  if(!owner)
  {
    // Pure early-return: group controllers only commit on attached members, so
    // there is nothing to store and no signal to emit when there is no owner.
    return;
  }

  mSelected = selected;
  IntegrationView::SetState(GetImpl(owner), ViewState::SELECTED, selected, event);
  mSelectionChangedSignal.Emit(owner, mSelected, event);
  OnSelectionChanged(owner, selected, event);
}

bool SelectableTraitImpl::IsToggleByClickEnabled() const
{
  return mToggleByClickEnabled;
}

void SelectableTraitImpl::EnableToggleByClick(bool enabled)
{
  if(mToggleByClickEnabled == enabled)
  {
    return;
  }

  mToggleByClickEnabled = enabled;

  if(mAttached)
  {
    if(enabled)
    {
      EnsureClickableAndConnect();
    }
    else
    {
      DisconnectClickable();
    }
  }
}

View SelectableTraitImpl::GetOwner() const
{
  return mOwner.GetHandle();
}

void SelectableTraitImpl::OnAttached(TraitId id, View& view)
{
  DALI_ASSERT_ALWAYS(!(mOwner.GetHandle()) && "The trait can not be attached multiple target views");
  mOwner = view;

  mAttached = true;

  if(mToggleByClickEnabled)
  {
    EnsureClickableAndConnect();
  }

  // Apply the radio costume only when bound to a group. The bind/rebind/re-Add path
  // re-applies the same costume via ApplyRadioAccessibilityOnBind because OnAttached
  // does not run again there. Winner arbitration lives in
  // SelectionGroupImpl::ReconcileNewMember.
  if(mGroup)
  {
    ApplyRadioCostume();
  }
}

void SelectableTraitImpl::OnDetaching(TraitId id, View& view)
{
  // Leave the group before the base clears the owner (a destroy mid-transition
  // must cancel any pending transition this trait initiated).
  if(mGroup)
  {
    mGroup->UnregisterMember(*this);
  }
  DisconnectClickable();
  mAttached = false;
  mOwner.Reset();
}

void SelectableTraitImpl::OnViewDestroying(ViewImpl* viewImpl)
{
  if(mGroup)
  {
    mGroup->UnregisterMember(*this);
  }
}

bool SelectableTraitImpl::OnSelectionChanging(View view, bool newSelected)
{
  if(!mGroup)
  {
    // Ungrouped (a plain selectable, or after Remove): allow the change.
    return true;
  }
  return mGroup->RequestSelectionChange(*this, GetOwner(), newSelected);
}

void SelectableTraitImpl::OnSelectionChanged(View view, bool selected, InputEvent event)
{
  if(mGroup)
  {
    // Keep the accessibility CHECKED bit in lock-step with the committed state
    // (both winner true and loser false), then let the group flush its signal.
    WriteCheckedState(view, selected);
    mGroup->NotifyCommitted(*this, view, selected, event);
  }
  // Ungrouped: a standalone selectable must never touch accessibility.
}

void SelectableTraitImpl::EnsureClickableAndConnect()
{
  View owner = mOwner.GetHandle();
  if(!owner)
  {
    return;
  }

  // Get or create InteractiveTrait on the owner view
  InteractiveTrait clickable = owner.AsInteractive();
  if(clickable)
  {
    clickable.ClickedSignal().Connect(this, &SelectableTraitImpl::OnClickedForToggle);
  }
}

void SelectableTraitImpl::DisconnectClickable()
{
  View owner = mOwner.GetHandle();
  if(!owner)
  {
    return;
  }

  InteractiveTrait clickable = GetInteractiveTrait(GetImpl(owner));
  if(clickable)
  {
    clickable.ClickedSignal().Disconnect(this, &SelectableTraitImpl::OnClickedForToggle);
  }
}

void SelectableTraitImpl::OnClickedForToggle(View view, InputEvent event)
{
  SetSelectedInternal(!mSelected, event);
}

// ----------------------------------------------------------------------------
// Group collaboration points
// ----------------------------------------------------------------------------

SelectionGroup SelectableTraitImpl::GetGroup() const
{
  return mGroup ? SelectionGroup(mGroup.Get()) : SelectionGroup();
}

SelectionGroupImpl* SelectableTraitImpl::GetGroupImpl() const
{
  return mGroup.Get();
}

void SelectableTraitImpl::CommitSelectionFromGroup(bool selected, InputEvent event)
{
  // The group has already arbitrated this change; commit it without re-vetoing.
  CommitSelectedState(selected, event);
}

void SelectableTraitImpl::BindGroup(SelectionGroupImpl* group)
{
  mGroup = group;
}

void SelectableTraitImpl::UnbindGroup(SelectionGroupImpl* group)
{
  if(mGroup.Get() == group)
  {
    mGroup.Reset();
  }
}

void SelectableTraitImpl::ApplyRadioAccessibilityOnBind()
{
  // Re-apply the radio costume on the bind/rebind/re-Add path. OnAttached does not run
  // again there (the trait stays attached, so re-running it would trip the base's
  // single-owner assert), so the group calls this in place to re-acquire RADIO_BUTTON
  // and re-seed CHECKED. Re-captures mPreviousRole from the role live just before this
  // bind, so a later Remove restores that role rather than a frozen first-attach value.
  ApplyRadioCostume();
}

void SelectableTraitImpl::ResetAccessibilityOnRemove()
{
  View owner = GetOwner();
  if(owner)
  {
    // Undo the radio costume: clear CHECKED while the role is still RADIO_BUTTON, then
    // restore the role captured at the last (re)bind. The selected bool is left as-is so
    // the View stays an ordinary selectable rather than becoming a deselected one.
    WriteCheckedState(owner, false);
    owner.SetProperty(Ui::View::Property::ACCESSIBILITY_ROLE, mPreviousRole);
  }
}

void SelectableTraitImpl::ApplyRadioCostume()
{
  View owner = GetOwner();
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

void SelectableTraitImpl::WriteCheckedState(View view, bool checked)
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

} // namespace Dali::Ui::Integration
