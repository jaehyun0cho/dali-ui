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

// INTERNAL INCLUDES
#include <dali-ui-foundation/public-api/input-event.h>
#include <dali-ui-foundation/public-api/selectable-trait.h>
#include <dali-ui-foundation/public-api/trait-object.h>

namespace Dali
{

namespace Ui
{

class InputEvent;

namespace Integration
{
/**
 * @brief Internal implementation of Selectable trait.
 *
 * SelectableTraitImpl manages the selected state of a View and optionally
 * toggles selection on click by listening to the owner View's InteractiveTrait.
 * Unlike InteractiveTraitImpl, this uses a separate reserved trait slot
 * (SELECTABLE_TRAIT).
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

protected:
  /**
   * @copydoc Dali::Ui::SelectableTrait::~SelectableTrait
   */
  virtual ~SelectableTraitImpl() override;

  /**
   * @brief Gets the owner view
   */
  View GetOwner() const;

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
   * Subclasses (e.g. GroupSelectableTraitImpl) can override this to
   * implement group deselection logic or to veto a selection change.
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
   * and the per-item SelectionChangedSignal has been emitted. Subclasses (e.g.
   * GroupSelectableTraitImpl) can override this to respond to a completed change,
   * for example to synchronise accessibility state or notify a group controller.
   * The base implementation is a no-op.
   *
   * @param[in] view The owner view
   * @param[in] selected The committed selection state
   * @param[in] event The input event that caused the change, or InputEvent::None()
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
   * @param[in] event The input event that caused the change, or InputEvent::None()
   */
  void CommitSelectedState(bool selected, InputEvent event);

private:
  void EnsureClickableAndConnect();
  void DisconnectClickable();
  void SetSelectedInternal(bool selected, InputEvent event);
  void OnClickedForToggle(View view, InputEvent event);

private:
  WeakHandle<View>                     mOwner;
  Signal<void(View, bool, InputEvent)> mSelectionChangedSignal;
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
