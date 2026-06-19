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
#include <string>

// INTERNAL INCLUDES
#include <dali-ui-foundation/public-api/dali-ui-common.h>
#include <dali-ui-foundation/public-api/selection-group.h>

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
 * @brief GroupSelectableTrait is a declarative trait that places a View's
 * selectable behavior into a single-selection SelectionGroup.
 *
 * GroupSelectableTrait does NOT replace or inherit from SelectableTrait; it
 * composes the View's existing SelectableTrait (reusing it, never swapping it)
 * and occupies its own reserved trait slot. It is purely declarative grouping
 * sugar: the mutual-exclusion arbitration lives in the SelectableTrait, while
 * this trait only orchestrates group membership through SelectionGroup.
 *
 * Two grouping mechanisms are offered, with a fixed precedence:
 * - An explicit group name set via SetGroupName(): the View joins the named
 *   SelectionGroup (SelectionGroup::Find(name)). An explicit name always wins.
 * - Parent auto-grouping (on by default): when no explicit name is set, the
 *   View joins the SelectionGroup associated with its parent View
 *   (SelectionGroup::Find(parentView)) while it is connected to a scene.
 *
 * Attaching this trait (via View::AsGroupSelectable()) ensures the View also has
 * a SelectableTrait, so the View is selectable and participates in the group's
 * mutual exclusion. The imperative SelectionGroup::Add(view) path remains the
 * low-level API; this trait is the declarative layer over it.
 */
class DALI_UI_API GroupSelectableTrait : public BaseHandle
{
public: // Creation & Destruction
  /**
   * @brief Creates an uninitialized GroupSelectableTrait handle.
   */
  GroupSelectableTrait();

  /**
   * @brief Creates an initialized GroupSelectableTrait.
   *
   * Parent auto-grouping is enabled by default and no explicit group name is set.
   *
   * @return A handle to a newly allocated Dali resource
   */
  static GroupSelectableTrait New();

  /**
   * @brief Downcasts a handle to a GroupSelectableTrait handle.
   *
   * If the handle refers to a GroupSelectableTrait (e.g. a trait from GetTrait),
   * the downcast produces a valid handle. Otherwise the returned handle is uninitialized.
   *
   * @param[in] handle Handle to an object stored in a View's group-selectable trait slot
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
   * @brief Sets the explicit group name for this View.
   *
   * The View leaves any group it had joined (its previous named group, or its
   * parent-auto group) and joins the SelectionGroup looked up by @p name
   * (SelectionGroup::Find(name)). An explicit name takes precedence over
   * parent auto-grouping. Passing an empty string clears the explicit name; the
   * View then falls back to parent auto-grouping (if enabled) on its next scene
   * connection.
   *
   * @param[in] name The group name to join, or an empty string to clear it
   */
  void SetGroupName(const std::string& name);

  /**
   * @brief Returns the explicit group name set for this View.
   *
   * @return The explicit group name, or an empty string if none is set
   */
  std::string GetGroupName() const;

  /**
   * @brief Returns the SelectionGroup this View currently belongs to.
   *
   * When an explicit group name is set, this returns SelectionGroup::Find(name).
   * Otherwise it returns the group the View's SelectableTrait is bound to (which,
   * for a parent-auto-grouped member, is its parent's group), or an empty handle
   * if the View is not currently a member of any group.
   *
   * @return The owning SelectionGroup, or an empty handle if not in a group
   */
  SelectionGroup GetGroup() const;

public: // Not intended for application developers
  /**
   * @brief Creates a handle using the Internal implementation.
   *
   * @param[in] implementation The implementation
   */
  explicit GroupSelectableTrait(Integration::GroupSelectableTraitImpl* implementation);
};

} // namespace Ui

} // namespace Dali
