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
#include <dali-ui-foundation/internal/views/view/selection-group-impl.h>

// INTERNAL INCLUDES
#include <dali-ui-foundation/internal/views/view/group-selectable-trait-impl.h>
#include <dali-ui-foundation/public-api/group-selectable-trait.h>
#include <dali-ui-foundation/public-api/selectable-trait.h>
#include <dali-ui-foundation/public-api/view.h>

namespace Dali
{

namespace Ui
{

namespace Internal
{

IntrusivePtr<SelectionGroupImpl> SelectionGroupImpl::New()
{
  return IntrusivePtr<SelectionGroupImpl>(new SelectionGroupImpl());
}

SelectionGroupImpl::SelectionGroupImpl()
: mMembers(),
  mWinner(nullptr),
  mSelectedMemberChangedSignal()
{
}

SelectionGroupImpl::~SelectionGroupImpl() = default;

Signal<void(View, View, InputEvent)>& SelectionGroupImpl::SelectedMemberChangedSignal()
{
  return mSelectedMemberChangedSignal;
}

GroupSelectableTraitImpl* SelectionGroupImpl::GetTraitImpl(View member)
{
  // Read-only mapping: never upgrades a non-group View. Returns nullptr if the View
  // is not already GroupSelectable, so Remove() of a non-member is a clean no-op.
  if(!member || !member.IsGroupSelectable())
  {
    return nullptr;
  }

  GroupSelectableTrait trait = member.AsGroupSelectable();
  return trait ? &GetImpl(trait) : nullptr;
}

View SelectionGroupImpl::ResolveView(GroupSelectableTraitImpl* member)
{
  return member ? member->GetOwner() : View();
}

void SelectionGroupImpl::Add(View member)
{
  if(!member)
  {
    return;
  }

  // Ensure the View has GroupSelectableTrait (which implies Selectable and Interactive),
  // then ask the member's trait implementation to join this group. JoinGroup is the
  // single place that wires the strong trait->group reference, registers the member
  // (RegisterMember below), and applies the radio accessibility costume.
  GroupSelectableTrait trait = member.AsGroupSelectable();
  GetImpl(trait).JoinGroup(this);
}

void SelectionGroupImpl::Remove(View member)
{
  if(!member)
  {
    return;
  }

  GroupSelectableTraitImpl* impl = GetTraitImpl(member);
  if(!impl || FindMember(impl) < 0)
  {
    // Not a member of this group.
    return;
  }

  // Ask the member's trait to leave: it unregisters from this group's state, drops its
  // strong group reference, and restores accessibility.
  impl->LeaveGroup();
}

uint32_t SelectionGroupImpl::GetMemberCount() const
{
  // Members are unregistered deterministically (Remove / OnViewDestroying), so the
  // member list never holds dangling pointers.
  return static_cast<uint32_t>(mMembers.size());
}

View SelectionGroupImpl::GetSelectedMember() const
{
  return ResolveView(mWinner);
}

void SelectionGroupImpl::ClearSelection()
{
  if(!mWinner)
  {
    // Already empty.
    return;
  }

  // Deselect the winner programmatically. This funnels through the member trait's
  // selection change, which re-enters OnMemberDeselected() with member == winner,
  // clearing mWinner and emitting the change signal exactly once.
  View winner = mWinner->GetOwner();
  if(winner)
  {
    SelectableTrait selectable = winner.AsSelectable();
    if(selectable)
    {
      selectable.SetSelected(false);
    }
  }
}

void SelectionGroupImpl::RegisterMember(GroupSelectableTraitImpl* member)
{
  if(!member)
  {
    return;
  }

  if(FindMember(member) >= 0)
  {
    // Already a member; idempotent.
    return;
  }

  mMembers.push_back(member);

  // Reconcile the joining member's selected state against the single-selection
  // invariant. A View can already be selected when it joins (it may have been a plain
  // Selectable before). The existing winner always wins; a member joining an empty
  // group becomes the winner as a silent seed.
  View            ownerView  = member->GetOwner();
  SelectableTrait selectable = ownerView ? ownerView.AsSelectable() : SelectableTrait();
  if(selectable && selectable.IsSelected())
  {
    if(mWinner && mWinner != member)
    {
      // Group already has a winner: deselect the joining member (no group signal; the
      // winner is unchanged). The deselect routes through OnMemberDeselected, which is
      // ignored because member != mWinner.
      selectable.SetSelected(false);
    }
    else if(!mWinner)
    {
      // Empty group: the joining member becomes the winner as a seed (no signal).
      mWinner = member;
    }
  }
}

void SelectionGroupImpl::UnregisterMember(GroupSelectableTraitImpl* member)
{
  if(!member)
  {
    return;
  }

  const int32_t index = FindMember(member);
  if(index >= 0)
  {
    mMembers.erase(mMembers.begin() + index);
  }

  // If the removed member was the winner, the group becomes empty (no promotion of
  // another member). A structural winner removal is a real selection change, so emit
  // SelectedMemberChangedSignal(previous, empty) ONCE -- but only while the previous
  // View is still resolvable. During View destruction OnViewDestroying runs with the
  // member's WeakHandle<View> already empty (ResolveView -> empty View), which both
  // skips the emit (signalling mid-destruction is unsafe/meaningless) and prevents a
  // dangling-View signal. Remove(winner) and a cross-group move both keep the View
  // alive, so they emit (A, empty) exactly once. The cause is Programmatic because the
  // removal is an API/structural action, not a gesture.
  if(mWinner == member)
  {
    View previousView = ResolveView(member);
    mWinner           = nullptr;
    if(previousView)
    {
      mSelectedMemberChangedSignal.Emit(previousView, View(), InputEvent::Programmatic());
    }
  }
}

void SelectionGroupImpl::OnMemberSelected(GroupSelectableTraitImpl* member, InputEvent event)
{
  if(!member)
  {
    return;
  }

  GroupSelectableTraitImpl* previous = mWinner;
  if(previous == member)
  {
    // Defensive: the member is already the winner. Nothing to swap.
    return;
  }

  // Record the NEW winner BEFORE deselecting the old one. This bounds re-entrancy:
  // deselecting the previous winner re-enters OnMemberDeselected with member == the old
  // winner, which is now != mWinner, so it is ignored. The group signal therefore fires
  // exactly once, from here.
  mWinner = member;

  View previousView = ResolveView(previous);
  if(previous && previousView)
  {
    SelectableTrait selectable = previousView.AsSelectable();
    if(selectable)
    {
      selectable.SetSelected(false);
    }
  }

  // Re-entrancy guard: deselecting the previous winner (and any user callback it runs)
  // can re-enter this method and elect a different winner. Only the emit whose `member`
  // is still the current winner is the terminal, non-stale change; a superseded inner
  // call must not emit a current== that is no longer selected.
  //
  // NOTE (phantom-previous limitation): in a nested user-callback cascade the surviving
  // terminal emit's `previous` was captured before the inner cascade ran, so it may name
  // a member that was never itself announced as `current`. The contract defines
  // previous/current per individual change, not cross-emission continuity, so this is a
  // documented limitation rather than a bug. A depth-counter/transaction is intentionally
  // NOT added.
  if(mWinner == member)
  {
    mSelectedMemberChangedSignal.Emit(previousView, ResolveView(member), event);
  }
}

void SelectionGroupImpl::OnMemberDeselected(GroupSelectableTraitImpl* member, InputEvent event)
{
  if(!member)
  {
    return;
  }

  if(mWinner != member)
  {
    // The displaced old winner during a swap, or a non-winner; ignore.
    return;
  }

  // The current winner was deselected (programmatic SetSelected(false) or
  // ClearSelection): the group becomes empty.
  View memberView = ResolveView(member);
  mWinner         = nullptr;
  mSelectedMemberChangedSignal.Emit(memberView, View(), event);
}

int32_t SelectionGroupImpl::FindMember(GroupSelectableTraitImpl* member) const
{
  for(std::size_t i = 0; i < mMembers.size(); ++i)
  {
    if(mMembers[i] == member)
    {
      return static_cast<int32_t>(i);
    }
  }
  return -1;
}

} // namespace Internal

Internal::SelectionGroupImpl& GetImpl(SelectionGroup& obj)
{
  BaseObject& baseObject = obj.GetBaseObject();
  return static_cast<Internal::SelectionGroupImpl&>(baseObject);
}

const Internal::SelectionGroupImpl& GetImpl(const SelectionGroup& obj)
{
  const BaseObject& baseObject = obj.GetBaseObject();
  return static_cast<const Internal::SelectionGroupImpl&>(baseObject);
}

} // namespace Ui

} // namespace Dali
