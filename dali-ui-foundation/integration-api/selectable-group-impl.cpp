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
#include <dali-ui-foundation/integration-api/selectable-group-impl.h>

// EXTERNAL INCLUDES
#include <dali/integration-api/debug.h>
#include <dali/public-api/common/dali-common.h>
#include <algorithm>

// INTERNAL INCLUDES
#include <dali-ui-foundation/integration-api/group-selectable-trait-impl.h>
#include <dali-ui-foundation/integration-api/reserved-trait-id.h>
#include <dali-ui-foundation/integration-api/view-integ.h>
#include <dali-ui-foundation/public-api/view-impl.h>
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
 * @brief Returns the selectable trait stored in the view's selectable slot, or an
 * empty handle. Does NOT create a trait if none exists.
 */
SelectableTrait GetSelectableTrait(View view)
{
  IntrusivePtr<TraitObject> object = IntegrationView::GetTrait(GetImpl(view), ReservedTraitId::SELECTABLE_TRAIT);
  auto*                     base   = dynamic_cast<BaseObject*>(object.Get());
  return base ? SelectableTrait::DownCast(BaseHandle(base)) : SelectableTrait();
}

} // unnamed namespace

// ============================================================================
// TransitionScope (dismiss-on-success scope guard)
// ============================================================================

SelectableGroupImpl::TransitionScope::TransitionScope(SelectableGroupImpl& group)
: mGroup(group),
  mDismissed(false)
{
  mGroup.mTransition = TransitionState::Transitioning;
}

SelectableGroupImpl::TransitionScope::~TransitionScope()
{
  if(!mDismissed)
  {
    // Intentionally does NOT touch mSelected. The cancel-on-detach path
    // (UnregisterMember) and ClearSelection/ReconcileNewMember each own mSelected for
    // their case; wiping it here would clear the legitimate winner during a
    // reconcile no-steal. See RequestSelectionChange for the mSelected ordering.
    mGroup.mTransition = TransitionState::Idle;
    mGroup.mPending.Reset();
  }
}

void SelectableGroupImpl::TransitionScope::Dismiss()
{
  mDismissed = true;
}

// ============================================================================
// SelectableGroupImpl
// ============================================================================

SelectableGroupImpl::SelectableGroupImpl()
: mMembers(),
  mSelected(),
  mSelectedMemberChangedSignal(),
  mPending(),
  mTransition(TransitionState::Idle),
  mAllowEmptySelection(true)
{
}

SelectableGroupImpl::~SelectableGroupImpl()
{
}

Signal<void(View, View, InputEvent)>& SelectableGroupImpl::SelectedMemberChangedSignal()
{
  return mSelectedMemberChangedSignal;
}

// ----------------------------------------------------------------------------
// Helpers
// ----------------------------------------------------------------------------

GroupSelectableTrait SelectableGroupImpl::FindMemberByView(View view) const
{
  if(!view)
  {
    return GroupSelectableTrait();
  }

  for(const auto& weak : mMembers)
  {
    GroupSelectableTrait member = weak.GetHandle();
    if(member && GetImpl(member).GetOwnerView() == view)
    {
      return member;
    }
  }
  return GroupSelectableTrait();
}

bool SelectableGroupImpl::IsMemberRegistered(GroupSelectableTraitImpl& trait) const
{
  for(const auto& weak : mMembers)
  {
    GroupSelectableTrait member = weak.GetHandle();
    if(member && &GetImpl(member) == &trait)
    {
      return true;
    }
  }
  return false;
}

void SelectableGroupImpl::RegisterMember(GroupSelectableTrait trait)
{
  if(!trait)
  {
    return;
  }
  if(!IsMemberRegistered(GetImpl(trait)))
  {
    mMembers.push_back(WeakHandle<GroupSelectableTrait>(trait));
  }
}

void SelectableGroupImpl::CompactMembers()
{
  mMembers.erase(
    std::remove_if(mMembers.begin(),
                   mMembers.end(),
                   [](const WeakHandle<GroupSelectableTrait>& weak)
  { return !weak.GetHandle(); }),
    mMembers.end());
}

void SelectableGroupImpl::DebugAssertSettled() const
{
  DALI_ASSERT_DEBUG(mTransition == TransitionState::Idle && !mPending.active &&
                    "SelectableGroup left a selection transition unsettled");
}

void SelectableGroupImpl::ReconcileNewMember(GroupSelectableTrait trait)
{
  if(!trait)
  {
    return;
  }
  GroupSelectableTraitImpl& impl = GetImpl(trait);

  if(!trait.IsSelected())
  {
    // Not selected: registered only. CHECKED is seeded false by OnAttached.
    return;
  }

  GroupSelectableTrait currentWinner = mSelected.GetHandle();
  if(!currentWinner)
  {
    // Selected and the group has no winner yet: this member becomes the winner.
    mSelected = WeakHandle<GroupSelectableTrait>(trait);
  }
  else if(&GetImpl(currentWinner) != &impl)
  {
    // Selected, but the group already has a different winner. Keep the existing
    // winner and deselect this member; no group signal. Guarded so the item-level
    // callbacks cannot re-enter a public mutation mid-reconcile.
    TransitionScope guard(*this);
    impl.CommitSelectionFromGroup(false, InputEvent::None());
  }
  // else: this member already is the winner; nothing to do.
}

// ----------------------------------------------------------------------------
// Public API
// ----------------------------------------------------------------------------

bool SelectableGroupImpl::Add(View member)
{
  if(!member)
  {
    return false;
  }
  if(mTransition == TransitionState::Transitioning)
  {
    return false;
  }

  SelectableTrait existing = GetSelectableTrait(member);

  if(!existing)
  {
    // No selectable trait: create a group trait, attach it, register, reconcile.
    GroupSelectableTrait      trait = GroupSelectableTrait::New(SelectableGroup(this));
    IntrusivePtr<TraitObject> traitObject(static_cast<TraitObject*>(&GetImpl(trait)));
    IntegrationView::SetTrait(GetImpl(member), ReservedTraitId::SELECTABLE_TRAIT, traitObject);
    RegisterMember(trait);
    ReconcileNewMember(trait);
    DebugAssertSettled();
    return true;
  }

  GroupSelectableTrait groupTrait = GroupSelectableTrait::DownCast(existing);
  if(groupTrait)
  {
    // Already a group trait: rebind to this group if it belongs to another.
    GroupSelectableTraitImpl& impl    = GetImpl(groupTrait);
    SelectableGroupImpl*      current = impl.GetGroupImpl();
    if(current != this)
    {
      if(current)
      {
        current->UnregisterMember(impl);
      }
      impl.BindGroup(this);

      // Re-apply the radio costume: OnAttached does not run again on rebind/re-Add, so
      // the trait would otherwise keep the role/CHECKED state left by the previous
      // Remove. This re-captures the pre-bind role, restores RADIO_BUTTON, and seeds
      // CHECKED from the member's preserved selected bool BEFORE arbitration. The
      // no-steal path in ReconcileNewMember then drives CHECKED back to false last; the
      // winner path leaves the seeded CHECKED intact. Plain property writes only, so the
      // state machine stays settled. Guarded on current != this: a redundant Add of a
      // View already in this group is a no-op, so re-applying (which would re-capture
      // mPreviousRole as the current RADIO_BUTTON and defeat the Remove role restore) is
      // skipped.
      impl.ApplyRadioAccessibilityOnBind();
    }
    RegisterMember(groupTrait);
    ReconcileNewMember(groupTrait);
    DebugAssertSettled();
    return true;
  }

  // A plain SelectableTrait is present: do not replace it (would lose state and
  // existing signal subscribers).
  DALI_LOG_ERROR(
    "SelectableGroup::Add failed: the View already has a plain SelectableTrait. "
    "Add the View to the group BEFORE calling View::AsSelectable() so the group can "
    "attach a GroupSelectableTrait; a plain SelectableTrait is never replaced (its "
    "selected state and signal subscribers would otherwise be lost).\n");
  return false;
}

bool SelectableGroupImpl::Remove(View member)
{
  if(mTransition == TransitionState::Transitioning)
  {
    return false;
  }

  GroupSelectableTrait trait = FindMemberByView(member);
  if(!trait)
  {
    return false;
  }
  GroupSelectableTraitImpl& impl = GetImpl(trait);

  for(auto it = mMembers.begin(); it != mMembers.end(); ++it)
  {
    GroupSelectableTrait m = it->GetHandle();
    if(m && &GetImpl(m) == &impl)
    {
      mMembers.erase(it);
      break;
    }
  }

  // If it was the winner, the group now has no selection. The member's selected
  // bool is intentionally preserved (it becomes a standalone selectable), but its
  // radio accessibility is undone below so it no longer announces as a radio button.
  GroupSelectableTrait winner = mSelected.GetHandle();
  if(winner && &GetImpl(winner) == &impl)
  {
    mSelected = WeakHandle<GroupSelectableTrait>();
  }

  // Undo the RADIO_BUTTON role and CHECKED bit while the relationship is intact.
  // This opens no transition, so the state machine stays settled.
  impl.ResetAccessibilityOnRemove();

  impl.UnbindGroup(this);
  DebugAssertSettled();
  return true;
}

uint32_t SelectableGroupImpl::GetMemberCount() const
{
  uint32_t count = 0;
  for(const auto& weak : mMembers)
  {
    if(weak.GetHandle())
    {
      ++count;
    }
  }
  return count;
}

View SelectableGroupImpl::GetSelectedMember() const
{
  // Outside a transition this is the committed winner. If queried mid-swap (e.g. from
  // the previous winner's deselect handler), mSelected already holds the prospective
  // new winner (set in RequestSelectionChange before the loser is deselected so the
  // cancel-on-detach path can recognise it); that is the value that will commit unless
  // the transition is cancelled.
  GroupSelectableTrait winner = mSelected.GetHandle();
  return winner ? GetImpl(winner).GetOwnerView() : View();
}

bool SelectableGroupImpl::SelectMember(View member)
{
  if(mTransition == TransitionState::Transitioning)
  {
    return false;
  }

  GroupSelectableTrait trait = FindMemberByView(member);
  if(!trait)
  {
    return false;
  }

  // Route through the normal selection path; the group arbitrates via
  // RequestSelectionChange and completes via NotifyCommitted.
  trait.SetSelected(true);
  DebugAssertSettled();

  // The selection can be aborted mid-transition if a signal handler detaches or
  // destroys the member; in that case the member is not selected. Report the
  // actual post-settle state so callers see false on an aborted (or otherwise
  // failed) selection and true on success, including an idempotent re-select.
  return GetImpl(trait).IsSelected();
}

bool SelectableGroupImpl::ClearSelection()
{
  if(mTransition == TransitionState::Transitioning)
  {
    return false;
  }

  GroupSelectableTrait winner = mSelected.GetHandle();
  if(!winner)
  {
    return true; // already empty
  }
  if(!mAllowEmptySelection)
  {
    return false;
  }

  View prevView = GetImpl(winner).GetOwnerView();
  {
    // Single synchronous frame: the guard resets to Idle on scope exit (no dismiss).
    TransitionScope guard(*this);
    mSelected = WeakHandle<GroupSelectableTrait>();
    mPending.Reset();
    // Winner item false + CHECKED false. Its post-commit NotifyCommitted is gated
    // out (selected == false / no pending), so it does not emit the group signal.
    GetImpl(winner).CommitSelectionFromGroup(false, InputEvent::None());
  }

  // Idle again here: emit the group signal directly (previous -> empty).
  mSelectedMemberChangedSignal.Emit(prevView, View(), InputEvent::None());
  DebugAssertSettled();
  return true;
}

void SelectableGroupImpl::SetAllowEmptySelection(bool allow)
{
  mAllowEmptySelection = allow;
}

bool SelectableGroupImpl::IsEmptySelectionAllowed() const
{
  return mAllowEmptySelection;
}

// ----------------------------------------------------------------------------
// Trait collaboration points
// ----------------------------------------------------------------------------

bool SelectableGroupImpl::RequestSelectionChange(GroupSelectableTraitImpl& trait, View ownerView, bool newSelected)
{
  if(mTransition == TransitionState::Transitioning)
  {
    return false; // reject external re-entry during a transition
  }

  GroupSelectableTrait current = mSelected.GetHandle();

  if(newSelected)
  {
    if(current && &GetImpl(current) == &trait)
    {
      return true; // idempotent (the base no-op guard also covers this)
    }

    GroupSelectableTrait traitHandle(&trait);

    TransitionScope guard(*this);

    // Record the pending transition (and the prospective winner) BEFORE deselecting
    // the previous winner. Deselecting it emits the loser's per-item signal
    // synchronously, and an application handler may detach or destroy the initiator
    // from there. With mPending already active, that teardown routes through
    // UnregisterMember, which cancels the transition (resets to Idle and clears
    // mSelected) instead of leaving it permanently stranded.
    mPending.previous  = current ? GetImpl(current).GetOwnerView() : View();
    mPending.current   = ownerView;
    mPending.initiator = WeakHandle<GroupSelectableTrait>(traitHandle);
    mPending.active    = true;
    mSelected          = WeakHandle<GroupSelectableTrait>(traitHandle);

    if(current)
    {
      // Deselect the previous winner. Its own post-commit notification is gated out
      // (selected == false; see NotifyCommitted).
      GetImpl(current).CommitSelectionFromGroup(false, InputEvent::None());
    }

    // If deselecting the previous winner cancelled this transition (the initiator was
    // detached or destroyed by an application handler), abort: leave the undismissed
    // scope guard to reset the group to Idle and keep its state consistent.
    if(!mPending.active)
    {
      return false;
    }

    // Normal completion: carry Transitioning across to the base's winner commit,
    // which the winner's post-commit NotifyCommitted will finish and reset.
    guard.Dismiss();
    return true;
  }

  // External deselect (re-click or direct SetSelected(false)).
  if(!current || &GetImpl(current) != &trait)
  {
    return true; // not the winner: harmless (the base no-op guard applies)
  }
  return false; // winner-deselect: always veto (no-op), regardless of allowEmpty
}

void SelectableGroupImpl::NotifyCommitted(GroupSelectableTraitImpl& trait, View ownerView, bool selected, InputEvent event)
{
  if(mTransition != TransitionState::Transitioning)
  {
    return;
  }
  if(!mPending.active)
  {
    return;
  }
  if(!selected)
  {
    return; // loser / clear false commits do not flush the group signal
  }

  GroupSelectableTrait initiator = mPending.initiator.GetHandle();
  if(!initiator || &GetImpl(initiator) != &trait)
  {
    return; // not the initiator that opened this transition
  }

  View previous = mPending.previous;
  View current  = mPending.current;

  // Consume and go Idle BEFORE emitting, so a group-signal handler may start a
  // fresh nested swap.
  mPending.Reset();
  mTransition = TransitionState::Idle;

  mSelectedMemberChangedSignal.Emit(previous, current, event);
}

void SelectableGroupImpl::UnregisterMember(GroupSelectableTraitImpl& trait)
{
  for(auto it = mMembers.begin(); it != mMembers.end(); ++it)
  {
    GroupSelectableTrait m = it->GetHandle();
    if(m && &GetImpl(m) == &trait)
    {
      mMembers.erase(it);
      break;
    }
  }
  CompactMembers();

  // Cancel an in-flight transition initiated by this trait (or whose initiator is
  // already gone), to prevent a stranded Transitioning state.
  if(mPending.active)
  {
    GroupSelectableTrait initiator = mPending.initiator.GetHandle();
    if(!initiator || &GetImpl(initiator) == &trait)
    {
      mPending.Reset();
      mTransition = TransitionState::Idle;
    }
  }

  // If it was the current winner, the group now has no selection. No automatic
  // promotion and no group signal during cleanup.
  GroupSelectableTrait winner = mSelected.GetHandle();
  if(winner && &GetImpl(winner) == &trait)
  {
    mSelected = WeakHandle<GroupSelectableTrait>();
  }
}

} // namespace Integration

} // namespace Ui

} // namespace Dali
