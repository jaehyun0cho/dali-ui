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
#include <cstdint>

// INTERNAL INCLUDES
#include <dali-ui-foundation/public-api/dali-ui-common.h>
#include <dali-ui-foundation/public-api/input-event.h>

namespace Dali
{

namespace Ui
{

// Forward declaration (view.h provides the full definition when needed).
// SelectionGroup deliberately does not include view.h to avoid a circular include
// (view.h pulls in the trait headers, which reach SelectionGroup through the trait).
class View;

namespace Internal
{
class SelectionGroupImpl;
}

/**
 * @brief SelectionGroup enforces single-selection (radio) semantics across a set of Views.
 *
 * A SelectionGroup is an explicit cross-View collaborator. At most one member of the
 * group can be selected at a time. When a member becomes selected, the previously
 * selected member is automatically deselected.
 *
 * Membership is explicit: a View joins a group by passing it to Add(). Adding a View
 * makes it a GroupSelectable member (it gains GroupSelectableTrait behavior, which
 * implies SelectableTrait and InteractiveTrait). Clicking a member always selects it;
 * clicking the already-selected member is a no-op (true radio behavior). A gesture can
 * never empty the group. The group does become empty (no selected member) through any of
 * these non-gesture routes: ClearSelection(), a programmatic SetSelected(false) on the
 * selected member's SelectableTrait, or Remove() of the currently selected member.
 *
 * The group does not extend its members' lifetime: it tracks members by raw, non-owning
 * pointers and auto-unregisters each member when it is destroyed, so it never holds a
 * dangling pointer.
 *
 * @code
 *   SelectionGroup group = SelectionGroup::New();
 *   group.Add(radioA);
 *   group.Add(radioB);
 *   group.SelectedMemberChangedSignal().Connect(&MyClass::OnRadioChanged);
 * @endcode
 */
class DALI_UI_API SelectionGroup : public BaseHandle
{
public: // Creation & Destruction
  /**
   * @brief Creates an uninitialized SelectionGroup handle.
   *
   * Use SelectionGroup::New() to create an initialized object.
   */
  SelectionGroup();

  /**
   * @brief Creates a new SelectionGroup.
   *
   * @return A handle to a newly created SelectionGroup
   */
  static SelectionGroup New();

  /**
   * @brief Downcasts a handle to a SelectionGroup handle.
   *
   * If the handle points to a SelectionGroup object, the downcast produces a valid
   * handle. Otherwise the returned handle is uninitialized.
   *
   * @param[in] handle A handle to an object
   * @return A handle to a SelectionGroup object or an uninitialized handle
   */
  static SelectionGroup DownCast(BaseHandle handle);

  /**
   * @brief Copy constructor.
   *
   * @param[in] handle A reference to the copied handle
   */
  SelectionGroup(const SelectionGroup& handle);

  /**
   * @brief Move constructor.
   *
   * @param[in] rhs A reference to the moved handle
   */
  SelectionGroup(SelectionGroup&& rhs) noexcept;

  /**
   * @brief Copy assignment operator.
   *
   * @param[in] handle A reference to the copied handle
   * @return A reference to this
   */
  SelectionGroup& operator=(const SelectionGroup& handle);

  /**
   * @brief Move assignment operator.
   *
   * @param[in] rhs A reference to the moved handle
   * @return A reference to this
   */
  SelectionGroup& operator=(SelectionGroup&& rhs) noexcept;

  /**
   * @brief Destructor.
   */
  ~SelectionGroup();

public: // Signals
  /**
   * @brief Emitted when the selected member of the group changes.
   *
   * The callback signature is: void YourCallbackName(View previous, View current, InputEvent event)
   *
   * @c previous is the member that was selected before the change (an empty View
   * handle if there was none). @c current is the member that is now selected (an
   * empty View handle if the group is now empty). @c event carries the originating
   * input cause, or InputEvent::Programmatic() for API-driven changes.
   *
   * @return The selected-member-changed signal
   */
  Signal<void(View, View, InputEvent)>& SelectedMemberChangedSignal();

public: // API
  /**
   * @brief Adds a View as a member of this group.
   *
   * The View becomes a GroupSelectable member (GroupSelectableTrait, which implies
   * SelectableTrait and InteractiveTrait, is attached if not already present). The
   * member gains the RADIO_BUTTON accessibility role.
   *
   * If the View is already a member of this group, this call is a no-op. If the View
   * is currently a member of another group, it is first removed from that group.
   *
   * If the View is already selected when it joins a group that already has a selected
   * member, the joining View is deselected to preserve the single-selection invariant.
   *
   * @param[in] member The View to add to the group
   */
  void Add(View member);

  /**
   * @brief Removes a View from this group.
   *
   * The View's GroupSelectable membership is released and its accessibility role is
   * restored to the value it had before joining (typically NONE). The member's own
   * selected state is preserved: IsSelected() and the SELECTED visual are unchanged.
   *
   * The accessibility CHECKED bit follows the restored role: it is meaningful only for
   * checkable roles (CHECK_BOX/RADIO_BUTTON/TOGGLE_BUTTON), so after the role is restored
   * to a non-checkable role (the typical NONE) the CHECKED bit is cleared even though the
   * logical SELECTED state is kept.
   *
   * If the removed member was the group's selected member, the group becomes empty (no
   * member is promoted) and SelectedMemberChangedSignal(previous, empty) is emitted once.
   *
   * If the View is not a member of this group, this call is a no-op.
   *
   * @param[in] member The View to remove from the group
   */
  void Remove(View member);

  /**
   * @brief Returns the number of live members currently in the group.
   *
   * @return The member count
   */
  uint32_t GetMemberCount() const;

  /**
   * @brief Returns the currently selected member of the group.
   *
   * @return The selected member, or an empty View handle if the group has no
   * selected member
   */
  View GetSelectedMember() const;

  /**
   * @brief Clears the current selection, leaving the group with no selected member.
   *
   * This is one of the routes to an empty group (the others are a programmatic
   * SetSelected(false) on the selected member and Remove() of the selected member); a
   * gesture never empties the group. If the group already has no selected member, this
   * call is a no-op.
   */
  void ClearSelection();

public: // Not intended for application developers
  /// @cond internal
  /**
   * @brief Allows the creation of a SelectionGroup handle from an internal pointer.
   *
   * @note Not intended for application developers
   * @param[in] implementation A pointer to the internal implementation
   */
  explicit DALI_INTERNAL SelectionGroup(Internal::SelectionGroupImpl* implementation);
  /// @endcond
};

} // namespace Ui

} // namespace Dali
