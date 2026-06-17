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

// INTERNAL INCLUDES
#include <dali-ui-foundation/integration-api/selectable-trait-impl.h>
#include <dali-ui-foundation/public-api/group-selectable-trait.h>
#include <dali-ui-foundation/public-api/input-event.h>
#include <dali-ui-foundation/public-api/selectable-group.h>

namespace Dali
{

namespace Ui
{

class View;
class ViewImpl;

namespace Integration
{

class SelectableGroupImpl;

/**
 * @brief Internal implementation of GroupSelectableTrait.
 *
 * Extends SelectableTraitImpl to route selection changes through a
 * SelectableGroupImpl, which enforces single selection across the group.
 *
 * Cross-class collaboration with the group uses only the public methods below
 * (there is no friendship): the group calls CommitSelectionFromGroup / GetOwnerView /
 * BindGroup / UnbindGroup, and this trait calls back into the group through
 * RequestSelectionChange / NotifyCommitted / UnregisterMember.
 */
class DALI_UI_API GroupSelectableTraitImpl : public SelectableTraitImpl
{
public:
  /**
   * @brief Constructor.
   *
   * @param[in] group The group that arbitrates this trait, held with a strong reference
   */
  explicit GroupSelectableTraitImpl(SelectableGroupImpl* group);

public: // Group collaboration points (group-internal use only)
  /**
   * @brief Commits a selection state requested by the group, bypassing the veto.
   *
   * Group-internal only: the group has already arbitrated the change, so it must
   * not be re-vetoed. Forwards to the protected SelectableTraitImpl::CommitSelectedState,
   * which does not consult OnSelectionChanging.
   *
   * @param[in] selected The selection state to commit
   * @param[in] event The originating input event, or InputEvent::None()
   */
  void CommitSelectionFromGroup(bool selected, InputEvent event);

  /**
   * @brief Returns the owner view of this trait.
   * @return The owner View, or an empty handle if not attached
   */
  View GetOwnerView() const;

  /**
   * @brief Binds this trait to the given group (strong reference).
   * @param[in] group The group to bind to
   */
  void BindGroup(SelectableGroupImpl* group);

  /**
   * @brief Unbinds this trait from the given group, if currently bound to it.
   * @param[in] group The group to unbind from
   */
  void UnbindGroup(SelectableGroupImpl* group);

  /**
   * @brief (Re)applies the radio-specific accessibility for this trait.
   *
   * Called by SelectableGroup::Add on the rebind/re-Add path, where OnAttached does
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
   * @brief Undoes the radio-specific accessibility applied by OnAttached / bind.
   *
   * Called by SelectableGroup::Remove when this trait leaves the group. Restores
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
  SelectableGroupImpl* GetGroupImpl() const;

  /**
   * @brief Returns the group handle this trait is bound to.
   * @return The bound group handle, or an empty handle
   */
  SelectableGroup GetGroup() const;

protected:
  /**
   * @brief Destructor.
   */
  ~GroupSelectableTraitImpl() override;

  /**
   * @copydoc SelectableTraitImpl::OnSelectionChanging
   */
  bool OnSelectionChanging(View view, bool newSelected) override;

  /**
   * @copydoc SelectableTraitImpl::OnSelectionChanged
   */
  void OnSelectionChanged(View view, bool selected, InputEvent event) override;

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

private:
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
  IntrusivePtr<SelectableGroupImpl> mGroup;        ///< STRONG (trait -> group); group -> member is weak, so there is no cycle
  int32_t                           mPreviousRole; ///< ACCESSIBILITY_ROLE captured on each (re)bind (skipped when the live role is our own RADIO_BUTTON costume), restored on Remove
};

} // namespace Integration

inline DALI_UI_API Integration::GroupSelectableTraitImpl& GetImpl(GroupSelectableTrait& obj)
{
  return static_cast<Integration::GroupSelectableTraitImpl&>(obj.GetBaseObject());
}

inline DALI_UI_API const Integration::GroupSelectableTraitImpl& GetImpl(const GroupSelectableTrait& obj)
{
  return static_cast<const Integration::GroupSelectableTraitImpl&>(obj.GetBaseObject());
}

} // namespace Ui

} // namespace Dali
