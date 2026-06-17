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

// INTERNAL INCLUDES
#include <dali-ui-foundation/public-api/dali-ui-common.h>
#include <dali-ui-foundation/public-api/selectable-group.h>
#include <dali-ui-foundation/public-api/selectable-trait.h>

namespace Dali
{

namespace Ui
{

// Forward declarations
namespace Integration
{
class GroupSelectableTraitImpl;
}

/**
 * @brief GroupSelectableTrait is a SelectableTrait whose selection is arbitrated
 * by a SelectableGroup to enforce single selection across the group.
 *
 * It shares the same reserved selectable trait slot as SelectableTrait, so a View
 * has either a plain SelectableTrait or a GroupSelectableTrait, never both.
 * Instances are created and attached through SelectableGroup::Add(); applications
 * do not construct them directly.
 *
 * While a member is the current winner, deselecting it from outside the group
 * (for example by re-clicking it or calling SetSelected(false)) is a no-op. Use
 * SelectableGroup::ClearSelection() to leave the group with no selection.
 */
class DALI_UI_API GroupSelectableTrait : public SelectableTrait
{
public: // Creation & Destruction
  /**
   * @brief Creates an uninitialized GroupSelectableTrait handle.
   */
  GroupSelectableTrait();

  /**
   * @brief Downcasts a handle to a GroupSelectableTrait handle.
   *
   * @param[in] handle Handle to an object stored in a View's selectable trait slot
   * @return A handle to GroupSelectableTrait or an uninitialized handle
   */
  static GroupSelectableTrait DownCast(BaseHandle handle);

  /**
   * @brief Copy constructor.
   *
   * Creates another handle that points to the same real object.
   * @param[in] groupSelectableTrait Handle to copy
   */
  GroupSelectableTrait(const GroupSelectableTrait& groupSelectableTrait);

  /**
   * @brief Destructor.
   *
   * Non-virtual since derived Handle types must not contain data or virtual methods.
   */
  ~GroupSelectableTrait();

public: // API
  /**
   * @brief Returns the group that arbitrates this trait's selection.
   *
   * The returned handle is for inspection only; do not use it to change this
   * trait's membership.
   *
   * @return The owning SelectableGroup, or an empty handle if not bound to a group
   */
  SelectableGroup GetGroup() const;

public: // Not intended for application developers
  /**
   * @brief Creates a GroupSelectableTrait bound to the given group.
   *
   * Internal: used by SelectableGroup::Add(). Two independent mechanisms apply:
   * declaring any New in this derived handle hides the inherited SelectableTrait::New()
   * by C++ name hiding, so GroupSelectableTrait::New() taking no argument is a compile
   * error and a group-bound trait can never be created ungrouped; DALI_INTERNAL is
   * unrelated to that — it only keeps this overload out of the exported ABI so code
   * outside the library cannot link against it.
   *
   * @param[in] group The group that will arbitrate this trait
   * @return A handle to the new GroupSelectableTrait
   */
  DALI_INTERNAL static GroupSelectableTrait New(SelectableGroup group);

  /**
   * @brief Creates a handle using the Internal implementation.
   *
   * @param[in] implementation The implementation
   */
  explicit GroupSelectableTrait(Integration::GroupSelectableTraitImpl* implementation);
};

} // namespace Ui

} // namespace Dali
