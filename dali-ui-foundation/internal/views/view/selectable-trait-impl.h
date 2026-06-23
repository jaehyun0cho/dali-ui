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
#include <dali/public-api/object/weak-handle.h>
#include <dali/public-api/signals/connection-tracker.h>
#include <dali/public-api/signals/dali-signal.h>

// INTERNAL INCLUDES
#include <dali-ui-foundation/public-api/input-event.h>
#include <dali-ui-foundation/public-api/selectable-trait.h>

namespace Dali
{

namespace Ui
{

class InputEvent;

namespace Internal
{
class CoreInteractionObject;

/**
 * @brief Internal implementation of Selectable trait.
 *
 * SelectableTraitImpl is stored inside CoreInteractionObject.
 * It manages the selected state of a View and optionally
 * toggles selection on click by listening to the owner View's InteractiveTrait.
 * It is installed as the selectable trait implementation inside CoreInteractionObject.
 */
class SelectableTraitImpl : public ConnectionTracker
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
   * @brief Sets the selection state, carrying the originating input cause.
   *
   * Internal-only overload (not exposed on the public SelectableTrait handle) used by
   * collaborators such as GroupSelectableTraitImpl to preserve the real click cause
   * instead of substituting InputEvent::Programmatic().
   *
   * @param[in] selected True to select, false to deselect
   * @param[in] event    The originating input cause
   */
  void SetSelected(bool selected, InputEvent event);

  /**
   * @copydoc Dali::Ui::SelectableTrait::IsToggleByClickEnabled
   */
  bool IsToggleByClickEnabled() const;

  /**
   * @copydoc Dali::Ui::SelectableTrait::EnableToggleByClick
   *
   * @note While toggle-by-click is locked (see SetToggleByClickLocked), this is a no-op:
   * external enable/disable requests are ignored until the lock is cleared.
   */
  void EnableToggleByClick(bool enabled);

  /**
   * @brief Locks or unlocks toggle-by-click.
   *
   * While locked, EnableToggleByClick() is ignored (a no-op), so the current
   * toggle-by-click wiring cannot be changed from outside. This is a generic mechanism:
   * the caller forces toggle-by-click to the desired value, then locks it to keep it
   * pinned, and unlocks before restoring the prior value. It carries no knowledge of why
   * the lock is held.
   *
   * @param[in] locked True to lock (EnableToggleByClick ignored), false to unlock
   */
  void SetToggleByClickLocked(bool locked);

  /**
   * @copydoc Dali::Ui::SelectableTrait::~SelectableTrait
   */
  virtual ~SelectableTraitImpl();

protected:
  friend class CoreInteractionObject;

  /**
   * @brief Gets the owner view
   */
  View GetOwner() const;

  void OnAttached(View& view);

  void OnDetaching(View& view);

  void OnViewDestroying(ViewImpl* viewImpl);

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
  bool                                 mToggleByClickLocked : 1; ///< While set, EnableToggleByClick() is a no-op.
  bool                                 mAttached : 1;
};

} // namespace Internal

Internal::SelectableTraitImpl&       GetImpl(SelectableTrait& obj);
const Internal::SelectableTraitImpl& GetImpl(const SelectableTrait& obj);

} // namespace Ui

} // namespace Dali
