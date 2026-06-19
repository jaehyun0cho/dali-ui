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
#include <dali/public-api/object/base-object.h>
#include <dali/public-api/object/weak-handle.h>
#include <dali/public-api/signals/dali-signal.h>

// INTERNAL INCLUDES
#include <dali-ui-foundation/public-api/input-event.h>
#include <dali-ui-foundation/public-api/selectable-trait.h>
#include <dali-ui-foundation/public-api/selection-group.h>
#include <dali-ui-foundation/public-api/trait-object.h>

namespace Dali
{

namespace Ui
{

class InputEvent;

namespace Integration
{

class SelectionGroupImpl;

/**
 * @brief Internal implementation of Selectable trait.
 *
 * SelectableTraitImpl manages the selected state of a View and optionally
 * toggles selection on click by listening to the owner View's InteractiveTrait.
 * Unlike InteractiveTraitImpl, this uses a separate reserved trait slot
 * (SELECTABLE_TRAIT).
 *
 * A SelectableTraitImpl may optionally be bound to a SelectionGroupImpl, which
 * arbitrates single selection (mutual exclusion) across a set of members and
 * applies radio-button accessibility. When ungrouped the trait is an ordinary
 * standalone selectable and never touches accessibility. Cross-class
 * collaboration with the group uses only the public methods below (there is no
 * friendship): the group calls CommitSelectionFromGroup / BindGroup / UnbindGroup /
 * ApplyRadioAccessibilityOnBind / ResetAccessibilityOnRemove, and this trait calls
 * back into the group through RequestSelectionChange / NotifyCommitted /
 * UnregisterMember.
 */
class DALI_UI_API SelectableTraitImpl : public TraitObject, public ConnectionTracker
{
public:
  /**
   * @copydoc Dali::Ui::SelectableTrait::SelectableTrait
   */
  SelectableTraitImpl();

public: // Signals
  /**
   * @copydoc Dali::Ui::SelectableTrait::SelectionChangedSignal
   */
  Signal<void(View, bool, InputEvent)>& SelectionChangedSignal();

public: // API
  /**
   * @copydoc Dali::Ui::SelectableTrait::IsSelected
   */
  bool IsSelected() const;

  /**
   * @copydoc Dali::Ui::SelectableTrait::SetSelected
   */
  void SetSelected(bool selected);

  /**
   * @copydoc Dali::Ui::SelectableTrait::IsToggleByClickEnabled
   */
  bool IsToggleByClickEnabled() const;

  /**
   * @copydoc Dali::Ui::SelectableTrait::EnableToggleByClick
   */
  void EnableToggleByClick(bool enabled);

  /**
   * @copydoc Dali::Ui::SelectableTrait::GetGroup
   */
  SelectionGroup GetGroup() const;

public: // Group collaboration points (group-internal use only)
  /**
   * @brief Commits a selection state requested by the group, bypassing the veto.
   *
   * Group-internal only: the group has already arbitrated the change, so it must
   * not be re-vetoed. Forwards to the protected CommitSelectedState, which does
   * not consult OnSelectionChanging.
   *
   * @param[in] selected The selection state to commit
   * @param[in] event The originating input event, or InputEvent()
   */
  void CommitSelectionFromGroup(bool selected, InputEvent event);

  /**
   * @brief Binds this trait to the given group (strong reference).
   * @param[in] group The group to bind to
   */
  void BindGroup(SelectionGroupImpl* group);

  /**
   * @brief Unbinds this trait from the given group, if currently bound to it.
   * @param[in] group The group to unbind from
   */
  void UnbindGroup(SelectionGroupImpl* group);

  /**
   * @brief (Re)applies the radio-specific accessibility for this trait.
   *
   * Called by SelectionGroup::Add on the bind/rebind/re-Add path, where OnAttached does
   * NOT run again (the trait is still attached, so re-running OnAttached would trip
   * its single-owner assert). Re-captures the host's current ACCESSIBILITY_ROLE (so
   * a later Remove restores the role live just before THIS bind) UNLESS that live role
   * is already our own RADIO_BUTTON costume (a cross-group rebind has no intervening
   * Remove, so capturing it would clobber the true original held in mPreviousRole),
   * applies RADIO_BUTTON when the host left the role at its default (NONE), and re-seeds
   * the CHECKED bit from the trait's committed selected state. Safe to call repeatedly
   * on an already attached trait: it performs plain property writes only and opens no
   * transition.
   */
  void ApplyRadioAccessibilityOnBind();

  /**
   * @brief Undoes the radio-specific accessibility applied by bind.
   *
   * Called by SelectionGroup::Remove when this trait leaves the group. Restores
   * the ACCESSIBILITY_ROLE captured at the last (re)bind and clears the CHECKED bit,
   * so a removed member no longer announces as a radio button. The base selectable
   * state (the selected bool and the base selectable commit path) is intentionally
   * left untouched: the View becomes an ordinary standalone selectable, not a
   * deselected one.
   */
  void ResetAccessibilityOnRemove();

  /**
   * @brief Returns the group implementation this trait is bound to, or nullptr.
   * @return The bound group implementation
   */
  SelectionGroupImpl* GetGroupImpl() const;

  /**
   * @brief Returns the owner view of this trait.
   * @return The owner View, or an empty handle if not attached
   */
  View GetOwner() const;

protected:
  /**
   * @copydoc Dali::Ui::SelectableTrait::~SelectableTrait
   */
  virtual ~SelectableTraitImpl() override;

  /**
   * @copydoc Dali::Ui::TraitObject::OnAttached
   */
  void OnAttached(TraitId id, View& view) override;

  /**
   * @copydoc Dali::Ui::TraitObject::OnDetaching
   */
  void OnDetaching(TraitId id, View& view) override;

  /**
   * @copydoc Dali::Ui::TraitObject::OnViewDestroying
   */
  void OnViewDestroying(ViewImpl* viewImpl) override;

  /**
   * @brief Called when the selection state is about to change.
   *
   * When bound to a group, routes the change through the group so it can enforce
   * single selection or veto the change. When ungrouped this always allows the
   * change (a plain selectable).
   *
   * @param[in] view The owner view
   * @param[in] newSelected The proposed new selection state
   * @return True to allow the change, false to reject it
   */
  virtual bool OnSelectionChanging(View view, bool newSelected);

  /**
   * @brief Called after a selection state change has been committed.
   *
   * Invoked by CommitSelectedState after the owner view state has been updated
   * and the per-item SelectionChangedSignal has been emitted. When bound to a group,
   * keeps the accessibility CHECKED bit in lock-step with the committed state and
   * notifies the group controller. When ungrouped this is a no-op (a standalone
   * selectable must never touch accessibility).
   *
   * @param[in] view The owner view
   * @param[in] selected The committed selection state
   * @param[in] event The input event that caused the change, or InputEvent()
   */
  virtual void OnSelectionChanged(View view, bool selected, InputEvent event);

  /**
   * @brief Commits a selection state change without consulting the OnSelectionChanging veto.
   *
   * Applies the new selection state to the owner view, emits SelectionChangedSignal,
   * then invokes OnSelectionChanged. This is the single internal commit point: it is
   * used by SetSelectedInternal once a change has passed the veto, and directly by
   * group controllers that have already arbitrated the change and must not be vetoed.
   *
   * Does nothing if the state is unchanged (avoids phantom signals on duplicate
   * commits) or if the trait is not attached to a view. Unlike SetSelectedInternal,
   * the unattached case is a pure early-return: it does not store the state, because
   * group controllers only commit on attached members.
   *
   * @param[in] selected The selection state to commit
   * @param[in] event The input event that caused the change, or InputEvent()
   */
  void CommitSelectedState(bool selected, InputEvent event);

private:
  void EnsureClickableAndConnect();
  void DisconnectClickable();
  void SetSelectedInternal(bool selected, InputEvent event);
  void OnClickedForToggle(View view, InputEvent event);

  /**
   * @brief (Re)captures the host role and applies the radio costume in place.
   *
   * Single source of truth shared by OnAttached and ApplyRadioAccessibilityOnBind:
   * captures mPreviousRole from the current owner (skipping the capture when the live
   * role is already our own RADIO_BUTTON costume, so a cross-group rebind preserves the
   * true original), sets RADIO_BUTTON only when the captured role is the default NONE,
   * then seeds CHECKED from IsSelected(). A no-op when there is no owner.
   */
  void ApplyRadioCostume();

  /**
   * @brief Writes the accessibility CHECKED bit, preserving all other state bits.
   */
  void WriteCheckedState(View view, bool checked);

private:
  WeakHandle<View>                     mOwner;
  Signal<void(View, bool, InputEvent)> mSelectionChangedSignal;
  IntrusivePtr<SelectionGroupImpl>     mGroup;        ///< STRONG (trait -> group); group -> member is weak, so there is no cycle. Null when ungrouped.
  int32_t                              mPreviousRole; ///< ACCESSIBILITY_ROLE captured on each (re)bind (skipped when the live role is our own RADIO_BUTTON costume), restored on Remove
  bool                                 mSelected : 1;
  bool                                 mToggleByClickEnabled : 1;
  bool                                 mAttached : 1;
};

} // namespace Integration

inline DALI_UI_API Integration::SelectableTraitImpl& GetImpl(SelectableTrait& obj)
{
  return static_cast<Integration::SelectableTraitImpl&>(obj.GetBaseObject());
}

inline DALI_UI_API const Integration::SelectableTraitImpl& GetImpl(const SelectableTrait& obj)
{
  return static_cast<const Integration::SelectableTraitImpl&>(obj.GetBaseObject());
}

} // namespace Ui

} // namespace Dali
