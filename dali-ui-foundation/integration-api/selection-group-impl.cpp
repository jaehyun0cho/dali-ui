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
#include <dali-ui-foundation/integration-api/selection-group-impl.h>

// EXTERNAL INCLUDES
#include <dali/public-api/common/dali-common.h>
#include <algorithm>

// INTERNAL INCLUDES
#include <dali-ui-foundation/integration-api/reserved-trait-id.h>
#include <dali-ui-foundation/integration-api/selectable-trait-impl.h>
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

SelectionGroupImpl::TransitionScope::TransitionScope(SelectionGroupImpl& group)
: mGroup(group),
  mDismissed(false)
{
  mGroup.mTransition = TransitionState::Transitioning;
}

SelectionGroupImpl::TransitionScope::~TransitionScope()
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

void SelectionGroupImpl::TransitionScope::Dismiss()
{
  mDismissed = true;
}

// ============================================================================
// SelectionGroupImpl
// ============================================================================

SelectionGroupImpl::SelectionGroupImpl()
: mMembers(),
  mSelected(),
  mSelectedMemberChangedSignal(),
  mPending(),
  mTransition(TransitionState::Idle),
  mAllowEmptySelection(true)
{
}

SelectionGroupImpl::~SelectionGroupImpl()
{
}

Signal<void(View, View, InputEvent)>& SelectionGroupImpl::SelectedMemberChangedSignal()
{
  return mSelectedMemberChangedSignal;
}

// ----------------------------------------------------------------------------
// Helpers
// ----------------------------------------------------------------------------

SelectableTrait SelectionGroupImpl::FindMemberByView(View view) const
{
  if(!view)
  {
    return SelectableTrait();
  }

  for(const auto& weak : mMembers)
  {
    SelectableTrait member = weak.GetHandle();
    if(member && GetImpl(member).GetOwner() == view)
    {
      return member;
    }
  }
  return SelectableTrait();
}

bool SelectionGroupImpl::IsMemberRegistered(SelectableTraitImpl& trait) const
{
  for(const auto& weak : mMembers)
  {
    SelectableTrait member = weak.GetHandle();
    if(member && &GetImpl(member) == &trait)
    {
      return true;
    }
  }
  return false;
}

void SelectionGroupImpl::RegisterMember(SelectableTrait trait)
{
  if(!trait)
  {
    return;
  }
  if(!IsMemberRegistered(GetImpl(trait)))
  {
    mMembers.push_back(WeakHandle<SelectableTrait>(trait));
  }
}

void SelectionGroupImpl::CompactMembers()
{
  mMembers.erase(
    std::remove_if(mMembers.begin(),
                   mMembers.end(),
                   [](const WeakHandle<SelectableTrait>& weak)
  { return !weak.GetHandle(); }),
    mMembers.end());
}

void SelectionGroupImpl::DebugAssertSettled() const
{
  DALI_ASSERT_DEBUG(mTransition == TransitionState::Idle && !mPending.active &&
                    "SelectionGroup left a selection transition unsettled");
}

void SelectionGroupImpl::ReconcileNewMember(SelectableTrait trait)
{
  if(!trait)
  {
    return;
  }
  SelectableTraitImpl& impl = GetImpl(trait);

  if(!trait.IsSelected())
  {
    // Not selected: registered only. CHECKED is seeded false on bind.
    return;
  }

  SelectableTrait currentWinner = mSelected.GetHandle();
  if(!currentWinner)
  {
    // Selected and the group has no winner yet: this member becomes the winner.
    mSelected = WeakHandle<SelectableTrait>(trait);
  }
  else if(&GetImpl(currentWinner) != &impl)
  {
    // Selected, but the group already has a different winner. Keep the existing
    // winner and deselect this member; no group signal. Guarded so the item-level
    // callbacks cannot re-enter a public mutation mid-reconcile.
    TransitionScope guard(*this);
    impl.CommitSelectionFromGroup(false, InputEvent());
  }
  // else: this member already is the winner; nothing to do.
}

// ----------------------------------------------------------------------------
// Public API
// ----------------------------------------------------------------------------

bool SelectionGroupImpl::Add(View member)
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
    // No selectable trait: create one, attach it, then bind + apply costume, register,
    // reconcile. The trait is created ungrouped and attached first (so OnAttached runs
    // without touching accessibility), then bound via the single uniform bind path below.
    SelectableTrait           trait = SelectableTrait::New();
    IntrusivePtr<TraitObject> traitObject(static_cast<TraitObject*>(&GetImpl(trait)));
    IntegrationView::SetTrait(GetImpl(member), ReservedTraitId::SELECTABLE_TRAIT, traitObject);

    SelectableTraitImpl& impl = GetImpl(trait);
    impl.BindGroup(this);
    impl.ApplyRadioAccessibilityOnBind();
    RegisterMember(trait);
    ReconcileNewMember(trait);
    DebugAssertSettled();
    return true;
  }

  SelectableTraitImpl& impl    = GetImpl(existing);
  SelectionGroupImpl*  current = impl.GetGroupImpl();

  if(current != nullptr)
  {
    // Already a group-bound trait: rebind to this group if it belongs to another.
    if(current != this)
    {
      current->UnregisterMember(impl);
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
    RegisterMember(existing);
    ReconcileNewMember(existing);
    DebugAssertSettled();
    return true;
  }

  // A plain (ungrouped) SelectableTrait is present: bind it in place. The existing
  // trait object, its selected state, and all its signal subscribers are preserved;
  // only the group binding and the radio costume are added. Order-independent: a View
  // may be made selectable before or after being added to a group.
  impl.BindGroup(this);
  impl.ApplyRadioAccessibilityOnBind();
  RegisterMember(existing);
  ReconcileNewMember(existing);
  DebugAssertSettled();
  return true;
}

bool SelectionGroupImpl::Remove(View member)
{
  if(mTransition == TransitionState::Transitioning)
  {
    return false;
  }

  SelectableTrait trait = FindMemberByView(member);
  if(!trait)
  {
    return false;
  }
  SelectableTraitImpl& impl = GetImpl(trait);

  for(auto it = mMembers.begin(); it != mMembers.end(); ++it)
  {
    SelectableTrait m = it->GetHandle();
    if(m && &GetImpl(m) == &impl)
    {
      mMembers.erase(it);
      break;
    }
  }

  // If it was the winner, the group now has no selection. The member's selected
  // bool is intentionally preserved (it becomes a standalone selectable), but its
  // radio accessibility is undone below so it no longer announces as a radio button.
  SelectableTrait winner = mSelected.GetHandle();
  if(winner && &GetImpl(winner) == &impl)
  {
    mSelected = WeakHandle<SelectableTrait>();
  }

  // Undo the RADIO_BUTTON role and CHECKED bit while the relationship is intact.
  // This opens no transition, so the state machine stays settled.
  impl.ResetAccessibilityOnRemove();

  impl.UnbindGroup(this);
  DebugAssertSettled();
  return true;
}

uint32_t SelectionGroupImpl::GetMemberCount() const
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

View SelectionGroupImpl::GetSelectedMember() const
{
  // Outside a transition this is the committed winner. If queried mid-swap (e.g. from
  // the previous winner's deselect handler), mSelected already holds the prospective
  // new winner (set in RequestSelectionChange before the loser is deselected so the
  // cancel-on-detach path can recognise it); that is the value that will commit unless
  // the transition is cancelled.
  SelectableTrait winner = mSelected.GetHandle();
  return winner ? GetImpl(winner).GetOwner() : View();
}

bool SelectionGroupImpl::SelectMember(View member)
{
  if(mTransition == TransitionState::Transitioning)
  {
    return false;
  }

  SelectableTrait trait = FindMemberByView(member);
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

bool SelectionGroupImpl::ClearSelection()
{
  if(mTransition == TransitionState::Transitioning)
  {
    return false;
  }

  SelectableTrait winner = mSelected.GetHandle();
  if(!winner)
  {
    return true; // already empty
  }
  if(!mAllowEmptySelection)
  {
    return false;
  }

  View prevView = GetImpl(winner).GetOwner();
  {
    // Single synchronous frame: the guard resets to Idle on scope exit (no dismiss).
    TransitionScope guard(*this);
    mSelected = WeakHandle<SelectableTrait>();
    mPending.Reset();
    // Winner item false + CHECKED false. Its post-commit NotifyCommitted is gated
    // out (selected == false / no pending), so it does not emit the group signal.
    GetImpl(winner).CommitSelectionFromGroup(false, InputEvent());
  }

  // Idle again here: emit the group signal directly (previous -> empty).
  mSelectedMemberChangedSignal.Emit(prevView, View(), InputEvent());
  DebugAssertSettled();
  return true;
}

void SelectionGroupImpl::SetAllowEmptySelection(bool allow)
{
  mAllowEmptySelection = allow;
}

bool SelectionGroupImpl::IsEmptySelectionAllowed() const
{
  return mAllowEmptySelection;
}

// ----------------------------------------------------------------------------
// Trait collaboration points
// ----------------------------------------------------------------------------

bool SelectionGroupImpl::RequestSelectionChange(SelectableTraitImpl& trait, View ownerView, bool newSelected)
{
  if(mTransition == TransitionState::Transitioning)
  {
    return false; // reject external re-entry during a transition
  }

  SelectableTrait current = mSelected.GetHandle();

  if(newSelected)
  {
    if(current && &GetImpl(current) == &trait)
    {
      return true; // idempotent (the base no-op guard also covers this)
    }

    SelectableTrait traitHandle(&trait);

    TransitionScope guard(*this);

    // Record the pending transition (and the prospective winner) BEFORE deselecting
    // the previous winner. Deselecting it emits the loser's per-item signal
    // synchronously, and an application handler may detach or destroy the initiator
    // from there. With mPending already active, that teardown routes through
    // UnregisterMember, which cancels the transition (resets to Idle and clears
    // mSelected) instead of leaving it permanently stranded.
    mPending.previous  = current ? GetImpl(current).GetOwner() : View();
    mPending.current   = ownerView;
    mPending.initiator = WeakHandle<SelectableTrait>(traitHandle);
    mPending.active    = true;
    mSelected          = WeakHandle<SelectableTrait>(traitHandle);

    if(current)
    {
      // Deselect the previous winner. Its own post-commit notification is gated out
      // (selected == false; see NotifyCommitted).
      GetImpl(current).CommitSelectionFromGroup(false, InputEvent());
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

void SelectionGroupImpl::NotifyCommitted(SelectableTraitImpl& trait, View ownerView, bool selected, InputEvent event)
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

  SelectableTrait initiator = mPending.initiator.GetHandle();
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

void SelectionGroupImpl::UnregisterMember(SelectableTraitImpl& trait)
{
  for(auto it = mMembers.begin(); it != mMembers.end(); ++it)
  {
    SelectableTrait m = it->GetHandle();
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
    SelectableTrait initiator = mPending.initiator.GetHandle();
    if(!initiator || &GetImpl(initiator) == &trait)
    {
      mPending.Reset();
      mTransition = TransitionState::Idle;
    }
  }

  // If it was the current winner, the group now has no selection. No automatic
  // promotion and no group signal during cleanup.
  SelectableTrait winner = mSelected.GetHandle();
  if(winner && &GetImpl(winner) == &trait)
  {
    mSelected = WeakHandle<SelectableTrait>();
  }
}

} // namespace Integration

} // namespace Ui

} // namespace Dali
