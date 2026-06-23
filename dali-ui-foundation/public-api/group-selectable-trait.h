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
#include <dali-ui-foundation/public-api/selectable-trait.h>
#include <dali-ui-foundation/public-api/selection-group.h>

namespace Dali
{

namespace Ui
{

// Forward declarations
namespace Internal
{
class CoreInteractionObject;
}

/**
 * @brief GroupSelectableTrait is a state trait that provides single-selection
 * (mutual exclusion / radio) behavior to a View.
 *
 * GroupSelectable implies Selectable implies Interactive. A View with
 * GroupSelectableTrait also has SelectableTrait behavior (a boolean selected
 * state and SelectionChangedSignal) and InteractiveTrait behavior (click
 * handling, pressed state). GroupSelectableTrait derives from SelectableTrait
 * so callers can use the returned handle for grouping, selection, and
 * interactive APIs alike.
 *
 * A grouped member belongs to an explicit SelectionGroup that enforces "only
 * one selected". Membership is established through SelectionGroup::Add(View),
 * which internally ensures GroupSelectable on the View. Clicking the
 * already-selected member is a no-op (true radio); a group never becomes empty
 * through a gesture, only through a non-gesture route such as
 * SelectionGroup::ClearSelection(), a programmatic SetSelected(false) on the
 * selected member, or SelectionGroup::Remove() of the selected member.
 *
 * Internally the interactive, selectable, and group-selectable trait
 * implementations share the single core interaction trait slot.
 *
 * @note InteractiveTrait, SelectableTrait, and GroupSelectableTrait are facets of a
 * single shared interaction object on a View. Comparing handles with operator== compares
 * that same underlying object, and DownCast is presence-based (does the requested facet's
 * sub-implementation exist?) rather than identity-based.
 */
class DALI_UI_API GroupSelectableTrait : public SelectableTrait
{
public:
  // Typedefs

public: // Creation & Destruction
  /**
   * @brief Creates an uninitialized GroupSelectableTrait handle.
   */
  GroupSelectableTrait();

  /**
   * @brief Downcasts a handle to GroupSelectableTrait handle.
   *
   * If the handle refers to a GroupSelectableTrait, the downcast produces a
   * valid handle. Otherwise the returned handle is uninitialized.
   *
   * @param[in] handle Handle to an object stored in View's core interaction trait slot
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
   * This is non-virtual since derived Handle types must not contain data or virtual methods.
   */
  ~GroupSelectableTrait();

public: // API
  /**
   * @brief Returns the SelectionGroup this member is bound to.
   *
   * Membership is changed through SelectionGroup::Add() and
   * SelectionGroup::Remove(); this accessor is read-only.
   *
   * @return The bound SelectionGroup, or an uninitialized handle if unbound
   */
  SelectionGroup GetGroup() const;

public: // Not intended for application developers
  /**
   * @brief Creates an internal GroupSelectableTrait handle.
   *
   * The returned handle stores a CoreInteractionObject and owns the interactive,
   * selectable, and group-selectable trait implementations. Application
   * developers should obtain this trait through SelectionGroup::Add(View).
   *
   * @return A handle to a newly allocated GroupSelectableTrait.
   */
  DALI_INTERNAL static GroupSelectableTrait New();

  /**
   * @brief Creates a handle using the internal core interaction trait object.
   *
   * @param[in] container The core interaction trait object
   * @return A handle to GroupSelectableTrait
   */
  DALI_INTERNAL static GroupSelectableTrait New(Internal::CoreInteractionObject* container);

private:
  explicit DALI_INTERNAL GroupSelectableTrait(Internal::CoreInteractionObject* container);
};

} // namespace Ui

} // namespace Dali
