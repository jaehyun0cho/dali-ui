#pragma once

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

// EXTERNAL INCLUDES
#include <dali/public-api/object/base-object.h>
#include <dali/public-api/object/weak-handle.h>
#include <dali/public-api/signals/dali-signal.h>
#include <vector>

// INTERNAL INCLUDES
#include <dali-ui-foundation/public-api/input-event.h>
#include <dali-ui-foundation/public-api/selectable-trait.h>
#include <dali-ui-foundation/public-api/selection-group.h>
#include <dali-ui-foundation/public-api/view.h>

namespace Dali
{

namespace Ui
{

namespace Integration
{

class SelectableTraitImpl;

/**
 * @brief Internal implementation of SelectionGroup.
 *
 * Holds the set of group members weakly, tracks the current winner, and runs the
 * single-selection state machine. Members are SelectableTraitImpl instances bound
 * to this group; collaboration uses only their public methods (there is no
 * friendship).
 *
 * A swap (A -> B) emits, in order: A item false -> A CHECKED false ->
 * B item true -> B CHECKED true -> SelectedMemberChangedSignal(A, B).
 */
class DALI_UI_API SelectionGroupImpl : public BaseObject
{
public:
  /**
   * @brief Constructor. Exclusive selection; empty selection allowed by default.
   */
  SelectionGroupImpl();

  /**
   * @copydoc Dali::Ui::SelectionGroup::SelectedMemberChangedSignal
   */
  Signal<void(View, View, InputEvent)>& SelectedMemberChangedSignal();

  /**
   * @copydoc Dali::Ui::SelectionGroup::Add
   */
  bool Add(View member);

  /**
   * @copydoc Dali::Ui::SelectionGroup::Remove
   */
  bool Remove(View member);

  /**
   * @copydoc Dali::Ui::SelectionGroup::GetMemberCount
   */
  uint32_t GetMemberCount() const;

  /**
   * @copydoc Dali::Ui::SelectionGroup::GetSelectedMember
   */
  View GetSelectedMember() const;

  /**
   * @copydoc Dali::Ui::SelectionGroup::SelectMember
   */
  bool SelectMember(View member);

  /**
   * @copydoc Dali::Ui::SelectionGroup::ClearSelection
   */
  bool ClearSelection();

  /**
   * @copydoc Dali::Ui::SelectionGroup::SetAllowEmptySelection
   */
  void SetAllowEmptySelection(bool allow);

  /**
   * @copydoc Dali::Ui::SelectionGroup::IsEmptySelectionAllowed
   */
  bool IsEmptySelectionAllowed() const;

public: // Trait collaboration points (called by SelectableTraitImpl; not for general use)
  /**
   * @brief Arbitrates a selection change requested by a member trait.
   *
   * Called from SelectableTraitImpl::OnSelectionChanging. When approving a
   * selection it deselects the previous winner and records the pending transition,
   * which the winner's subsequent commit completes through NotifyCommitted.
   *
   * @param[in] trait The member trait requesting the change
   * @param[in] ownerView The trait's owner view
   * @param[in] newSelected The proposed new selection state
   * @return True to allow the change to proceed (commit), false to veto it
   */
  bool RequestSelectionChange(SelectableTraitImpl& trait, View ownerView, bool newSelected);

  /**
   * @brief Notified by a member trait after a selection change has been committed.
   *
   * Called from SelectableTraitImpl::OnSelectionChanged. Completes a pending
   * transition opened by the winner and emits SelectedMemberChangedSignal exactly once.
   *
   * @param[in] trait The member trait whose change has committed
   * @param[in] ownerView The trait's owner view
   * @param[in] selected The committed selection state
   * @param[in] event The originating input event, or InputEvent()
   */
  void NotifyCommitted(SelectableTraitImpl& trait, View ownerView, bool selected, InputEvent event);

  /**
   * @brief Removes a member trait that is detaching or whose view is being destroyed.
   *
   * Also cancels an in-flight transition initiated by this trait, preventing a
   * stranded Transitioning state.
   *
   * @param[in] trait The member trait to unregister
   */
  void UnregisterMember(SelectableTraitImpl& trait);

protected:
  /**
   * @brief Destructor.
   */
  ~SelectionGroupImpl() override;

private:
  enum class TransitionState
  {
    Idle,
    Transitioning
  };

  /**
   * @brief Pending transition state. The event is not stored: the group signal
   * uses the event delivered to the winner's post-commit hook.
   */
  struct Pending
  {
    View                        previous;
    View                        current;
    WeakHandle<SelectableTrait> initiator;
    bool                        active = false;

    void Reset()
    {
      previous  = View();
      current   = View();
      initiator = WeakHandle<SelectableTrait>();
      active    = false;
    }
  };

  /**
   * @brief Dismiss-on-success scope guard for the selection critical section.
   *
   * Arms Transitioning on construction. On destruction, unless Dismiss() was
   * called, it resets the transition to Idle and clears any pending state. This
   * gives exception safety, and lets single-frame clears reset automatically while
   * a swap carries the transition across to the winner's commit via Dismiss().
   */
  class TransitionScope
  {
  public:
    explicit TransitionScope(SelectionGroupImpl& group);
    ~TransitionScope();
    void Dismiss();

  private:
    SelectionGroupImpl& mGroup;
    bool                mDismissed;
  };

  // Helpers
  SelectableTrait FindMemberByView(View view) const;
  bool            IsMemberRegistered(SelectableTraitImpl& trait) const;
  void            RegisterMember(SelectableTrait trait);
  void            ReconcileNewMember(SelectableTrait trait);
  void            CompactMembers();

  // Debug-only tripwire: every top-level public mutation must leave the state
  // machine settled (Idle, no pending transition). No-op in release builds.
  void DebugAssertSettled() const;

private:
  std::vector<WeakHandle<SelectableTrait>> mMembers;  ///< WEAK references; null entries compacted lazily
  WeakHandle<SelectableTrait>              mSelected; ///< Current winner
  Signal<void(View, View, InputEvent)>     mSelectedMemberChangedSignal;
  Pending                                  mPending;
  TransitionState                          mTransition;
  bool                                     mAllowEmptySelection;
};

} // namespace Integration

inline DALI_UI_API Integration::SelectionGroupImpl& GetImpl(SelectionGroup& obj)
{
  return static_cast<Integration::SelectionGroupImpl&>(obj.GetBaseObject());
}

inline DALI_UI_API const Integration::SelectionGroupImpl& GetImpl(const SelectionGroup& obj)
{
  return static_cast<const Integration::SelectionGroupImpl&>(obj.GetBaseObject());
}

} // namespace Ui

} // namespace Dali
