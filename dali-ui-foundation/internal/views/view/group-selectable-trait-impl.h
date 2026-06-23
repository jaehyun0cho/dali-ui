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
#include <dali/public-api/common/intrusive-ptr.h>
#include <dali/public-api/object/weak-handle.h>
#include <dali/public-api/signals/connection-tracker.h>
#include <cstdint>

// INTERNAL INCLUDES
#include <dali-ui-foundation/public-api/group-selectable-trait.h>
#include <dali-ui-foundation/public-api/input-event.h>
#include <dali-ui-foundation/public-api/selection-group.h>

namespace Dali
{

namespace Ui
{

class InputEvent;

namespace Internal
{
class CoreInteractionObject;
class SelectionGroupImpl;

/**
 * @brief Internal implementation of GroupSelectable trait.
 *
 * GroupSelectableTraitImpl is stored inside CoreInteractionObject alongside the
 * InteractiveTraitImpl and SelectableTraitImpl. It implements single-selection
 * (radio / mutual exclusion) by collaborating with a SelectionGroupImpl through
 * signals only:
 *
 *  - The interaction wiring is GATED ON GROUP MEMBERSHIP. While the member is
 *    bound to a group it disables the sibling SelectableTrait's toggle-by-click
 *    and instead connects a SELECT-ONLY handler to the InteractiveTrait
 *    ClickedSignal. The handler always requests SetSelected(true), never
 *    toggles, so clicking the already-selected member is a free no-op (true
 *    radio) via the SelectableTraitImpl no-op guard. While the member is
 *    ungrouped (AsGroupSelectable() but not Add()ed, or after Remove()) it
 *    behaves exactly as a plain Selectable: toggle-by-click is left at its
 *    pre-join value and no select-only handler is installed.
 *  - The selection observer is MEMBERSHIP-INDEPENDENT: it stays connected to
 *    the sibling SelectableTrait SelectionChangedSignal for the whole attached
 *    lifetime, no-opping while mGroup is null. When this member becomes selected
 *    (and is grouped) it notifies its SelectionGroupImpl, which records the new
 *    winner and deselects the previous winner.
 *
 * It also keeps the member's accessibility in lock-step: RADIO_BUTTON role on
 * join, CHECKED state mirrored to the selected state via read-modify-write.
 */
class GroupSelectableTraitImpl : public ConnectionTracker
{
public:
  /**
   * @copydoc Dali::Ui::GroupSelectableTrait::GroupSelectableTrait
   */
  GroupSelectableTraitImpl();

public: // API
  /**
   * @copydoc Dali::Ui::GroupSelectableTrait::GetGroup
   */
  SelectionGroup GetGroup() const;

  /**
   * @copydoc Dali::Ui::GroupSelectableTrait::~GroupSelectableTrait
   */
  virtual ~GroupSelectableTraitImpl();

public: // SelectionGroupImpl collaboration
  // The following are public (unlike the base SelectableTraitImpl collaboration
  // helpers which are protected and reached through the friend CoreInteractionObject)
  // because the collaborator SelectionGroupImpl is a SEPARATE, non-friend class that
  // must call them from selection-group-impl.cpp.

  /**
   * @brief Binds this member to a group.
   *
   * Called by SelectionGroupImpl::Add(). Sets the strong group back-pointer, registers
   * this member with the group (which reconciles its selected state with the group's
   * current winner) and then, if attached, installs the membership-gated select-only
   * interaction wiring (capturing and forcing off toggle-by-click) and applies the radio
   * accessibility costume.
   *
   * @param[in] group The owning group implementation
   */
  void JoinGroup(SelectionGroupImpl* group);

  /**
   * @brief Unbinds this member from its group.
   *
   * Called by SelectionGroupImpl::Remove(). Unregisters from the group (which emits the
   * change signal if this member was the winner) and clears the group back-pointer; then,
   * if attached, reverts the interaction wiring (restoring the pre-join toggle-by-click)
   * and restores accessibility. The member's own selected state is preserved, so it
   * behaves once again as a plain Selectable.
   */
  void LeaveGroup();

  /**
   * @brief Gets the owner view.
   *
   * @return The owner view, or an uninitialized handle if not attached
   */
  View GetOwner() const;

protected:
  friend class CoreInteractionObject;

  void OnAttached(View& view);

  void OnDetaching(View& view);

  void OnViewDestroying(ViewImpl* viewImpl);

private:
  /**
   * @brief Connects the membership-INDEPENDENT selection observer.
   *
   * Connects SelectableTrait::SelectionChangedSignal -> OnSelectionChanged. The handler
   * no-ops while mGroup is null (if(mGroup) guard), so it is safe to keep connected for
   * the whole attached lifetime regardless of group membership.
   */
  void ConnectSelectionObserver();

  /**
   * @brief Disconnects the selection observer connected by ConnectSelectionObserver().
   */
  void DisconnectSelectionObserver();

  /**
   * @brief Installs the membership-GATED select-only interaction wiring.
   *
   * Captures the sibling SelectableTrait's current toggle-by-click value into
   * mPreviousToggleByClick (BEFORE forcing it off), disables toggle-by-click so a click
   * never deselects the winner, then connects InteractiveTrait::ClickedSignal ->
   * OnClickedForSelect. The group impl drives selection through this SELECT-ONLY handler.
   */
  void InstallInteraction();

  /**
   * @brief Reverts the interaction wiring installed by InstallInteraction().
   *
   * Disconnects this trait's own OnClickedForSelect handler, then restores the sibling
   * SelectableTrait's toggle-by-click to the saved mPreviousToggleByClick value (NOT
   * unconditionally true), so an ungrouped member behaves exactly as it did before it
   * joined the group.
   */
  void RevertInteraction();

  /**
   * @brief SELECT-ONLY click handler. Always requests SetSelected(true).
   */
  void OnClickedForSelect(View view, InputEvent event);

  /**
   * @brief Post-commit selection-changed handler.
   *
   * Mirrors CHECKED accessibility and notifies the group of the change.
   */
  void OnSelectionChanged(View view, bool selected, InputEvent event);

  /**
   * @brief Common group teardown shared by LeaveGroup(), OnViewDestroying(), and the
   * (dead) OnDetaching() path.
   *
   * Unregisters from the group on the stable trait-pointer identity and drops the
   * strong group reference. Accessibility restore is the caller's responsibility (it
   * is meaningful only while the owner View is alive, i.e. on LeaveGroup/Remove).
   */
  void UnbindFromGroup();

  /**
   * @brief Applies radio accessibility to the owner: caches the prior role,
   * sets RADIO_BUTTON, and seeds the CHECKED bit from the current selected state.
   */
  void ApplyRadioAccessibility();

  /**
   * @brief Restores the owner's accessibility: restores the cached role and sets the
   * CHECKED bit to follow the RESTORED role (CHECKED only for checkable roles
   * CHECK_BOX/RADIO_BUTTON/TOGGLE_BUTTON, and only when still selected). For the typical
   * NONE restored role this clears CHECKED. The logical SELECTED state is independent of
   * CHECKED, so this never unselects the member.
   */
  void RestoreAccessibility();

  /**
   * @brief Writes the CHECKED accessibility bit using read-modify-write so that
   * other state bits (e.g. ENABLED) are preserved.
   *
   * @param[in] view    The owner view
   * @param[in] checked The desired CHECKED bit value
   */
  void WriteCheckedState(View view, bool checked);

private:
  WeakHandle<View>                 mOwner;
  IntrusivePtr<SelectionGroupImpl> mGroup; ///< Strong reference; the group holds only a non-owning pointer back (no cycle).
  int32_t                          mPreviousRole;
  bool                             mRoleCached : 1;
  bool                             mAttached : 1;
  bool                             mPreviousToggleByClick : 1; ///< Sibling SelectableTrait toggle-by-click value captured at join, restored on leave.
};

} // namespace Internal

Internal::GroupSelectableTraitImpl&       GetImpl(GroupSelectableTrait& obj);
const Internal::GroupSelectableTraitImpl& GetImpl(const GroupSelectableTrait& obj);

} // namespace Ui

} // namespace Dali
