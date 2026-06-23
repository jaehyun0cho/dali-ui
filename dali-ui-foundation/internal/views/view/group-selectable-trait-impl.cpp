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
#include <dali-ui-foundation/internal/views/view/group-selectable-trait-impl.h>

// INTERNAL INCLUDES
#include <dali-ui-foundation/internal/views/view/core-interaction-object.h>
#include <dali-ui-foundation/internal/views/view/selectable-trait-impl.h>
#include <dali-ui-foundation/internal/views/view/selection-group-impl.h>
#include <dali-ui-foundation/internal/views/view/view-data-impl.h>
#include <dali-ui-foundation/public-api/input-event.h>
#include <dali-ui-foundation/public-api/interactive-trait.h>
#include <dali-ui-foundation/public-api/selectable-trait.h>
#include <dali-ui-foundation/public-api/view-accessibility-enums.h>
#include <dali-ui-foundation/public-api/view-impl.h>
#include <dali-ui-foundation/public-api/view.h>

namespace Dali::Ui::Internal
{
namespace
{
// Mirrors GetInteractiveTrait() in selectable-trait-impl.cpp:34-38, one level up:
// reach the sibling trait handle from the shared core interaction trait slot.
InteractiveTrait GetInteractiveTrait(ViewImpl& viewImpl)
{
  auto* traitObject = ViewDataImpl::Get(viewImpl).GetCoreInteractionObject();
  return traitObject ? InteractiveTrait::DownCast(BaseHandle(static_cast<BaseObject*>(traitObject))) : InteractiveTrait();
}

SelectableTrait GetSelectableTrait(ViewImpl& viewImpl)
{
  auto* traitObject = ViewDataImpl::Get(viewImpl).GetCoreInteractionObject();
  return traitObject ? SelectableTrait::DownCast(BaseHandle(static_cast<BaseObject*>(traitObject))) : SelectableTrait();
}

constexpr int32_t CHECKED_MASK = 1 << static_cast<int32_t>(AccessibilityState::CHECKED);

} // unnamed namespace

GroupSelectableTraitImpl::GroupSelectableTraitImpl()
: mOwner(),
  mGroup(),
  mPreviousRole(static_cast<int32_t>(AccessibilityRole::NONE)),
  mRoleCached(false),
  mAttached(false),
  mPreviousToggleByClick(false)
{
}

GroupSelectableTraitImpl::~GroupSelectableTraitImpl()
{
  // Defensive insurance: if this trait impl is destroyed while still bound to a group
  // (e.g. unexpected teardown order), drop the strong group reference and unregister so
  // the group never holds a dangling pointer. Idempotent; a no-op when already unbound.
  UnbindFromGroup();
}

SelectionGroup GroupSelectableTraitImpl::GetGroup() const
{
  return SelectionGroup(mGroup.Get());
}

View GroupSelectableTraitImpl::GetOwner() const
{
  return mOwner.GetHandle();
}

void GroupSelectableTraitImpl::JoinGroup(SelectionGroupImpl* group)
{
  // Idempotent re-join to the same group.
  if(mGroup.Get() == group)
  {
    return;
  }

  // Cross-group rebind: leave the previous group first (restores accessibility).
  if(mGroup)
  {
    LeaveGroup();
  }

  mGroup = group;
  if(!mGroup)
  {
    return;
  }

  // M2 ordering: register FIRST so the group's join reconciliation settles the
  // selected bit (a member joining a group that already has a winner is forced
  // false; a member joining an empty group becomes the seed winner). The
  // force-false path routes through OnSelectionChanged, which clears CHECKED.
  mGroup->RegisterMember(this);

  // THEN, while attached, install the membership-gated select-only interaction wiring
  // (captures and forces off toggle-by-click, connects OnClickedForSelect) and apply
  // the radio costume so CHECKED is seeded from the now-final selected state (the seed
  // winner ends up CHECKED, a displaced loser ends up not CHECKED) while the role is
  // RADIO_BUTTON.
  if(mAttached)
  {
    InstallInteraction();
    ApplyRadioAccessibility();
  }
}

void GroupSelectableTraitImpl::LeaveGroup()
{
  if(!mGroup)
  {
    return;
  }

  // Keep the group alive across mGroup.Reset() so the deferred change signal can be
  // emitted from it after the member is fully torn down.
  IntrusivePtr<SelectionGroupImpl> group = mGroup;

  // Structural removal ONLY (no signal here): erases this member and, if it was the
  // winner, clears the winner and returns the previous View (empty on the destroy path).
  // The signal is deferred to the end of this method so a re-add issued from the change
  // callback sees this member as a fully detached, plain Selectable.
  View previous = group->UnregisterMember(this);

  // Member side: no longer in a group. Done before reverting wiring so the selection
  // observer (which no-ops once mGroup is null) cannot re-enter group arbitration during
  // teardown.
  mGroup.Reset();

  if(mAttached)
  {
    // Revert the membership-gated interaction: unlock and restore the pre-join
    // toggle-by-click value and disconnect this trait's own OnClickedForSelect, so the
    // member behaves once again as a plain Selectable. THEN restore accessibility (role +
    // the CHECKED bit per the restored role, FIX-5).
    RevertInteraction();
    RestoreAccessibility();
  }

  if(previous)
  {
    // A structural winner removal is a real selection change. Announce it AFTER the
    // member is fully restored (group dropped, interaction reverted, accessibility
    // restored), so a re-add via group.Add() from this callback succeeds. The cause is
    // Programmatic because the removal is an API/structural action, not a gesture.
    group->EmitSelectedMemberChanged(previous, View(), InputEvent::Programmatic());
  }
}

void GroupSelectableTraitImpl::OnAttached(View& view)
{
  DALI_ASSERT_ALWAYS(!(mOwner.GetHandle()) && "The trait can not be attached multiple target views");
  mOwner    = view;
  mAttached = true;

  // Connect the membership-INDEPENDENT selection observer only. The interaction wiring
  // is membership-GATED and is installed by JoinGroup(); in practice mGroup is null here
  // because Add() -> AsGroupSelectable() attaches the trait BEFORE JoinGroup() runs. The
  // if(mGroup) branch below is defensive for a (currently impossible) attach-after-join.
  ConnectSelectionObserver();

  if(mGroup)
  {
    InstallInteraction();
    ApplyRadioAccessibility();
  }
}

void GroupSelectableTraitImpl::OnDetaching(View& view)
{
  // CoreInteractionObject is attach-once, so this is effectively a dead path
  // (core-interaction-object.cpp:131 asserts on detach; :100 is the attach-multiple
  // assert). All live teardown also happens in OnViewDestroying and
  // SelectionGroupImpl::Remove(). Kept for symmetry.
  if(mGroup)
  {
    UnbindFromGroup();
    RevertInteraction();
    RestoreAccessibility();
  }

  DisconnectSelectionObserver();
  mAttached = false;
  mOwner.Reset();
}

void GroupSelectableTraitImpl::OnViewDestroying(ViewImpl* viewImpl)
{
  // Runs from ~ViewImpl while the CoreInteractionObject (and this trait impl) are
  // still alive. mOwner.GetHandle() is already empty here, which is why the group is
  // keyed on the stable GroupSelectableTraitImpl* identity: UnregisterMember(this)
  // erases the exact entry and clears the winner deterministically. Accessibility
  // restore is meaningless mid-destruction, so it is intentionally skipped.
  UnbindFromGroup();
}

void GroupSelectableTraitImpl::UnbindFromGroup()
{
  if(mGroup)
  {
    // Destroy/detach path only (NOT LeaveGroup). Structural removal followed by dropping
    // the group reference. The returned View is intentionally discarded and NO change
    // signal is emitted: on the destroy path UnregisterMember returns an empty View
    // anyway (mOwner already resolves empty), and destruction must not emit.
    mGroup->UnregisterMember(this);
    mGroup.Reset();
  }
}

void GroupSelectableTraitImpl::ConnectSelectionObserver()
{
  View owner = mOwner.GetHandle();
  if(!owner)
  {
    return;
  }

  // Membership-INDEPENDENT: observe the sibling SelectableTrait's post-commit selection
  // change for the whole attached lifetime. OnSelectionChanged no-ops while mGroup is
  // null, so keeping it connected when ungrouped is harmless.
  SelectableTrait selectable = GetSelectableTrait(GetImpl(owner));
  DALI_ASSERT_ALWAYS(selectable && "GroupSelectableTraitImpl requires SelectableTrait");
  selectable.SelectionChangedSignal().Connect(this, &GroupSelectableTraitImpl::OnSelectionChanged);
}

void GroupSelectableTraitImpl::DisconnectSelectionObserver()
{
  View owner = mOwner.GetHandle();
  if(!owner)
  {
    return;
  }

  // Only ever disconnect the handler THIS trait connected; user-connected callbacks on
  // the same signal are untouched.
  SelectableTrait selectable = GetSelectableTrait(GetImpl(owner));
  if(selectable)
  {
    selectable.SelectionChangedSignal().Disconnect(this, &GroupSelectableTraitImpl::OnSelectionChanged);
  }
}

void GroupSelectableTraitImpl::InstallInteraction()
{
  View owner = mOwner.GetHandle();
  if(!owner)
  {
    return;
  }

  // Capture the sibling SelectableTrait toggle-by-click value BEFORE forcing it off, so
  // RevertInteraction() can restore exactly what the member had pre-join. Then disable
  // toggle so a click never deselects the winner (selectable-trait-impl.cpp:101-121); the
  // group impl drives selection through the SELECT-ONLY handler below instead.
  SelectableTrait selectable = GetSelectableTrait(GetImpl(owner));
  DALI_ASSERT_ALWAYS(selectable && "GroupSelectableTraitImpl requires SelectableTrait");
  mPreviousToggleByClick = selectable.IsToggleByClickEnabled();
  selectable.EnableToggleByClick(false);
  // Lock toggle-by-click AFTER forcing it off, so the off takes effect and the app cannot
  // re-enable OnClickedForToggle while grouped (a winner tap would otherwise toggle off
  // and empty the group). The lock is a generic SelectableTraitImpl mechanism with no
  // group knowledge; RevertInteraction() unlocks it on leave.
  GetImpl(selectable).SetToggleByClickLocked(true);

  InteractiveTrait clickable = GetInteractiveTrait(GetImpl(owner));
  DALI_ASSERT_ALWAYS(clickable && "GroupSelectableTraitImpl requires InteractiveTrait");
  clickable.ClickedSignal().Connect(this, &GroupSelectableTraitImpl::OnClickedForSelect);
}

void GroupSelectableTraitImpl::RevertInteraction()
{
  View owner = mOwner.GetHandle();
  if(!owner)
  {
    return;
  }

  // Disconnect this trait's OWN select-only handler (never a user-connected callback),
  // then restore the saved toggle-by-click value (NOT unconditionally true) so the
  // ungrouped member behaves exactly as it did before joining the group.
  InteractiveTrait clickable = GetInteractiveTrait(GetImpl(owner));
  if(clickable)
  {
    clickable.ClickedSignal().Disconnect(this, &GroupSelectableTraitImpl::OnClickedForSelect);
  }

  SelectableTrait selectable = GetSelectableTrait(GetImpl(owner));
  if(selectable)
  {
    // Unlock BEFORE restoring so the restore is honored (EnableToggleByClick is a no-op
    // while locked). The lock is the generic SelectableTraitImpl mechanism installed by
    // InstallInteraction().
    GetImpl(selectable).SetToggleByClickLocked(false);
    selectable.EnableToggleByClick(mPreviousToggleByClick);
  }
}

void GroupSelectableTraitImpl::OnClickedForSelect(View view, InputEvent event)
{
  // SELECT-ONLY: always request true, never toggle. Reclick on the already
  // selected winner is a free no-op via the SelectableTraitImpl guard
  // if(mSelected == selected) return; (selectable-trait-impl.cpp:76-79), so a
  // gesture can never empty the group.
  View owner = mOwner.GetHandle();
  if(!owner)
  {
    return;
  }

  SelectableTrait selectable = GetSelectableTrait(GetImpl(owner));
  if(selectable)
  {
    // Forward the originating click cause (Internal-only overload) so the group
    // signal carries the real gesture event rather than InputEvent::Programmatic().
    GetImpl(selectable).SetSelected(true, event);
  }
}

void GroupSelectableTraitImpl::OnSelectionChanged(View view, bool selected, InputEvent event)
{
  // Post-commit (SelectionChangedSignal is the last line of SetSelectedInternal,
  // selectable-trait-impl.cpp:88). Mirror CHECKED accessibility first.
  if(mGroup)
  {
    WriteCheckedState(view, selected);

    if(selected)
    {
      // The group records the new winner BEFORE deselecting the previous one, so
      // the displaced winner's re-entrant SelectionChanged(false) is ignored
      // (winner != self). Exactly one group signal fires, from OnMemberSelected.
      mGroup->OnMemberSelected(this, event);
    }
    else
    {
      // Reached only by programmatic SetSelected(false) on the winner or
      // SelectionGroupImpl::ClearSelection(); never by gesture.
      mGroup->OnMemberDeselected(this, event);
    }
  }
}

void GroupSelectableTraitImpl::ApplyRadioAccessibility()
{
  View owner = mOwner.GetHandle();
  if(!owner)
  {
    return;
  }

  if(!mRoleCached)
  {
    mPreviousRole = owner.GetProperty<int32_t>(View::Property::ACCESSIBILITY_ROLE);
    mRoleCached   = true;
  }

  owner.SetProperty(View::Property::ACCESSIBILITY_ROLE, static_cast<int32_t>(AccessibilityRole::RADIO_BUTTON));

  // Seed CHECKED from the member's current selected state, reusing the sibling-handle
  // helper (mirrors selectable-trait-impl.cpp:34-38) rather than an inline re-DownCast.
  SelectableTrait selectable = GetSelectableTrait(GetImpl(owner));
  WriteCheckedState(owner, selectable && selectable.IsSelected());
}

void GroupSelectableTraitImpl::RestoreAccessibility()
{
  View owner = mOwner.GetHandle();
  if(!owner)
  {
    return;
  }

  // Capture the role being restored BEFORE mRoleCached/mPreviousRole are reset below.
  // If the role was never cached (ApplyRadioAccessibility never ran), the member never
  // became RADIO_BUTTON, so fall back to the View's current role.
  const int32_t restoredRole = mRoleCached ? mPreviousRole : owner.GetProperty<int32_t>(View::Property::ACCESSIBILITY_ROLE);

  if(mRoleCached)
  {
    owner.SetProperty(View::Property::ACCESSIBILITY_ROLE, mPreviousRole);
    mRoleCached = false;
  }

  // CHECKED is meaningful only for checkable roles; after leaving the group the role is
  // restored (typically NONE), so the bit must follow the restored role, not raw
  // IsSelected(). The logical/visual SELECTED state is carried independently by
  // ViewState::SELECTED, so clearing CHECKED here does not unselect the member. For the
  // typical NONE case this yields CHECKED=false (same as the previous unconditional clear).
  const bool      checkable  = (restoredRole == static_cast<int32_t>(AccessibilityRole::CHECK_BOX) ||
                          restoredRole == static_cast<int32_t>(AccessibilityRole::RADIO_BUTTON) ||
                          restoredRole == static_cast<int32_t>(AccessibilityRole::TOGGLE_BUTTON));
  SelectableTrait selectable = GetSelectableTrait(GetImpl(owner));
  WriteCheckedState(owner, checkable && selectable && selectable.IsSelected());
}

void GroupSelectableTraitImpl::WriteCheckedState(View view, bool checked)
{
  if(!view)
  {
    return;
  }

  // ACCESSIBILITY_STATES is a whole-bitset INTEGER property whose SetProperty
  // replaces the value wholesale (view-data-impl.cpp). Use read-modify-write so
  // other bits (e.g. ENABLED) survive.
  int32_t raw = view.GetProperty<int32_t>(View::Property::ACCESSIBILITY_STATES);
  view.SetProperty(View::Property::ACCESSIBILITY_STATES, checked ? (raw | CHECKED_MASK) : (raw & ~CHECKED_MASK));
}

} // namespace Dali::Ui::Internal
