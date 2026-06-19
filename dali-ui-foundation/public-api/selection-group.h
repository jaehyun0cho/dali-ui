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
#include <dali/public-api/object/base-handle.h>
#include <dali/public-api/signals/dali-signal.h>
#include <string>

// INTERNAL INCLUDES
#include <dali-ui-foundation/public-api/dali-ui-common.h>
#include <dali-ui-foundation/public-api/input-event.h>

namespace Dali
{

namespace Ui
{

// Forward declaration (view.h provides full definition when needed)
class View;

// Forward declarations
namespace Integration
{
class SelectionGroupImpl;
}

/**
 * @brief SelectionGroup is a logical controller that enforces single selection
 * (mutual exclusion) across a set of selectable member Views.
 *
 * At most one member of a group is selected at a time. Selecting a member
 * automatically deselects the previous winner and emits SelectedMemberChangedSignal.
 *
 * A group is not part of the View hierarchy. Members join through Add() and are
 * tracked weakly, so a group never extends the lifetime of its members. Members
 * are Views backed by a group-bound SelectableTrait, which Add() attaches (or
 * binds in place) automatically.
 *
 * Re-clicking the currently selected member, or calling SetSelected(false) on it,
 * is a no-op: the only way to leave a group with no selection is ClearSelection(),
 * and only when empty selection is allowed.
 */
class DALI_UI_API SelectionGroup : public BaseHandle
{
public: // Creation & Destruction
  /**
   * @brief Creates an uninitialized SelectionGroup handle.
   */
  SelectionGroup();

  /**
   * @brief Creates a new exclusive SelectionGroup.
   *
   * The new group allows empty selection by default.
   *
   * @return A handle to a newly created SelectionGroup
   */
  static SelectionGroup New();

  /**
   * @brief Returns the SelectionGroup associated with the given name, creating it
   * on first lookup.
   *
   * The first lookup of a name implicitly creates a new SelectionGroup and registers
   * it under that name; later lookups of the same name return the same group while it
   * is still alive. The registry holds the group weakly, so once every handle to it
   * (and every member keeping it alive) is dropped, the entry is purged and a
   * subsequent Find() of the same name creates a fresh group.
   *
   * A freshly created group has GetMemberCount() == 0 and allows empty selection.
   *
   * @param[in] name The lookup name
   * @return The SelectionGroup associated with the name
   */
  static SelectionGroup Find(const std::string& name);

  /**
   * @brief Returns the SelectionGroup associated with the given parent View, creating
   * it on first lookup.
   *
   * The first lookup for a parent View implicitly creates a new SelectionGroup and
   * registers it against that View; later lookups for the same View return the same
   * group while it is still alive. Both the parent View and the group are held weakly,
   * so once either is gone the entry is purged and a subsequent Find() creates a fresh
   * group.
   *
   * This does not add the parent (or its children) to the group; it only provides a
   * stable per-parent group handle. A freshly created group has GetMemberCount() == 0
   * and allows empty selection.
   *
   * @param[in] parentView The parent View used as the lookup key
   * @return The SelectionGroup associated with the parent View
   */
  static SelectionGroup Find(View parentView);

  /**
   * @brief Downcasts a handle to a SelectionGroup handle.
   *
   * @param[in] handle Handle to an object
   * @return A handle to SelectionGroup or an uninitialized handle
   */
  static SelectionGroup DownCast(BaseHandle handle);

  /**
   * @brief Copy constructor.
   *
   * Creates another handle that points to the same real object.
   * @param[in] selectionGroup Handle to copy
   */
  SelectionGroup(const SelectionGroup& selectionGroup);

  /**
   * @brief Move constructor.
   * @param[in] rhs Handle to move
   */
  SelectionGroup(SelectionGroup&& rhs) noexcept;

  /**
   * @brief Copy assignment operator.
   * @param[in] rhs Handle to copy
   * @return Reference to this
   */
  SelectionGroup& operator=(const SelectionGroup& rhs);

  /**
   * @brief Move assignment operator.
   * @param[in] rhs Handle to move
   * @return Reference to this
   */
  SelectionGroup& operator=(SelectionGroup&& rhs) noexcept;

  /**
   * @brief Destructor.
   */
  ~SelectionGroup();

public: // Signals
  /**
   * @brief Emitted after the selected member of the group changes.
   *
   * The callback signature is:
   * void YourCallback(View previousSelected, View currentSelected, InputEvent event)
   *
   * An empty View means "no selection": previousSelected is empty for the first
   * selection, and currentSelected is empty after ClearSelection(). The event is
   * the input event that originated the change, or InputEvent() for
   * programmatic changes and clears.
   *
   * @note This signal fires only for selection changes (SelectMember, a click, or
   * ClearSelection). Membership operations never emit it, even when they change which
   * member the group reports as selected: Add() of the first selected member, Remove()
   * of the current winner, and cross-group rebind are all silent. Query
   * GetSelectedMember() after a membership change if you need the post-operation state.
   *
   * @note The group itself is not passed to the callback: a handler is connected
   * to a specific group's signal, so it already has that context. (Dali::Signal
   * supports at most three arguments.)
   *
   * @return The selected-member changed signal
   */
  Signal<void(View, View, InputEvent)>& SelectedMemberChangedSignal();

public: // API
  /**
   * @brief Adds a View as a member of this group.
   *
   * If the View has no selectable trait, one is created, attached, and bound to this
   * group. If it already has a SelectableTrait, that existing trait is bound to this
   * group in place: its selected state and signal subscribers are preserved, and only
   * the group binding plus the radio costume are added. If the trait is already bound
   * to another group, it is rebound to this group. Membership is therefore
   * order-independent: a View may be made selectable before or after being added.
   *
   * Adding an already-selected member never steals selection from an existing
   * winner: the existing winner is kept and the new member is deselected.
   *
   * Only a non-default (non-NONE) ACCESSIBILITY_ROLE set before Add() is preserved;
   * an explicit NONE is indistinguishable from the default and is replaced with
   * RADIO_BUTTON. The captured role is restored on Remove() and re-captured on each
   * Add()/rebind. Likewise, an application that deliberately sets ACCESSIBILITY_ROLE
   * to RADIO_BUTTON before Add() is indistinguishable from the group's own costume, so
   * Remove() restores NONE rather than the app's RADIO_BUTTON (same nature as the
   * explicit-NONE limitation above).
   *
   * @param[in] member The View to add
   * @return True if the View is a member of this group after the call; false if
   *         the View is empty or a selection transition is in progress
   */
  bool Add(View member);

  /**
   * @brief Removes a View from this group.
   *
   * The member's selected state is preserved, but its radio accessibility is undone:
   * the CHECKED bit is cleared and the ACCESSIBILITY_ROLE is restored to the role that
   * was live just before this Add() (re-captured on each Add()/rebind). The View
   * becomes an ordinary standalone selectable that no longer participates in mutual
   * exclusion, so it can then be toggled freely on its own.
   *
   * The round-trip is symmetric: re-Add() of a removed member (or a cross-group
   * rebind of the View) re-applies RADIO_BUTTON and re-seeds CHECKED from the member's
   * preserved selected state.
   *
   * @param[in] member The View to remove
   * @return True if the View was a member and was removed; false if it is not a
   *         member or a selection transition is in progress
   */
  bool Remove(View member);

  /**
   * @brief Returns the number of live, registered members.
   * @return The member count
   */
  uint32_t GetMemberCount() const;

  /**
   * @brief Returns the currently selected member, or an empty View if none.
   *
   * Outside a transition this is the committed winner. If queried from within a
   * per-item SelectionChangedSignal handler that fires during a swap (the previous
   * winner's deselect notification), it returns the prospective new winner — the value
   * that will be committed unless the transition is cancelled.
   *
   * @return The selected member View
   */
  View GetSelectedMember() const;

  /**
   * @brief Selects the given member, deselecting the previous winner.
   *
   * @param[in] member The member View to select
   * @return True if, after the call has fully settled, this member is the
   *         group's selected winner (this includes an idempotent re-select of
   *         the current winner); false otherwise - e.g. it is not a member, a
   *         selection transition is already in progress, the selection was
   *         aborted because the member was detached or destroyed from a signal
   *         handler fired during the transition, or a re-entrant handler fired
   *         during the transition left a different member selected
   */
  bool SelectMember(View member);

  /**
   * @brief Clears the current selection, leaving the group with no selected member.
   *
   * This is the only way to leave a group with no selection.
   *
   * @return True if the selection was cleared, or was already empty (clearing an
   *         already-empty group always succeeds); false only when there is a current
   *         winner and empty selection is not allowed, or a transition is in progress
   */
  bool ClearSelection();

  /**
   * @brief Sets whether the group is allowed to have no selected member.
   *
   * This governs whether ClearSelection() may leave the group empty. It does NOT
   * enable emptying the group by clicking the selected member: re-clicking the winner
   * (or calling SetSelected(false) on it) is always a no-op regardless of this setting.
   * With empty selection allowed (the default), ClearSelection() is the only route to
   * an empty group.
   *
   * @param[in] allow True to allow empty selection (the default), false otherwise
   */
  void SetAllowEmptySelection(bool allow);

  /**
   * @brief Returns whether the group is allowed to have no selected member.
   * @return True if empty selection is allowed
   */
  bool IsEmptySelectionAllowed() const;

public: // Not intended for application developers
  /**
   * @brief Creates a handle using the Internal implementation.
   *
   * @param[in] implementation The implementation
   */
  explicit SelectionGroup(Integration::SelectionGroupImpl* implementation);
};

} // namespace Ui

} // namespace Dali
