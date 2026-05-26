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
#include <dali-ui-foundation/public-api/dali-ui-common.h>
#include <dali-ui-foundation/public-api/view.h>
#include <cstdint>

namespace Dali
{

namespace Ui
{

namespace Internal
{
class NavigatorImpl;
}

#include "navigator.autogen.h"
/**
 * @brief Navigator manages regular and modal view stacks.
 *
 * Regular views are pushed on the navigation stack. Modal views are pushed on
 * the modal stack and are placed above regular navigation views.
 */
class DALI_UI_API Navigator : public View
{
public:
  /**
   * @brief Creates an uninitialized Navigator handle.
   */
  Navigator();

  /**
   * @brief Creates an initialized Navigator.
   *
   * @return A handle to a newly allocated Navigator
   */
  static Navigator New();

  /**
   * @brief Copy constructor.
   *
   * @param[in] navigator Handle to copy
   */
  Navigator(const Navigator& navigator);

  /**
   * @brief Move constructor.
   *
   * @param[in] rhs Handle to move
   */
  Navigator(Navigator&& rhs) noexcept;

  /**
   * @brief Destructor.
   */
  ~Navigator();

  /**
   * @brief Copy assignment operator.
   *
   * @param[in] handle Object to assign this to
   * @return Reference to this
   */
  Navigator& operator=(const Navigator& handle);

  /**
   * @brief Move assignment operator.
   *
   * @param[in] rhs Object to assign this to
   * @return Reference to this
   */
  Navigator& operator=(Navigator&& rhs) noexcept;

  /**
   * @brief Downcasts a handle to Navigator handle.
   *
   * @param[in] handle Handle to an object
   * @return A Navigator handle or an uninitialized handle
   */
  static Navigator DownCast(BaseHandle handle);

public: // Navigation API
  /**
   * @brief Gets the number of regular navigation views.
   *
   * @return The navigation stack count
   */
  uint32_t GetNavigationStackCount() const;

  /**
   * @brief Gets the number of modal views.
   *
   * @return The modal stack count
   */
  uint32_t GetModalStackCount() const;

  /**
   * @brief Gets a regular navigation view by stack index.
   *
   * @param[in] index The stack index
   * @return The view at the requested index, or an uninitialized handle
   */
  View GetNavigationStackAt(uint32_t index) const;

  /**
   * @brief Gets a modal view by stack index.
   *
   * @param[in] index The stack index
   * @return The view at the requested index, or an uninitialized handle
   */
  View GetModalStackAt(uint32_t index) const;

  /**
   * @brief Gets the current top view.
   *
   * Modal views take priority over regular navigation views.
   *
   * @return The current top view, or an uninitialized handle
   */
  View GetCurrentView() const;

  /**
   * @brief Pushes a view onto the regular navigation stack.
   *
   * @param[in] view The view to push
   * @param[in] animated Whether to use the default transition animation
   */
  void Push(View view, bool animated = true);

  /**
   * @brief Pushes a view onto the modal stack.
   *
   * @param[in] view The modal view to push
   * @param[in] animated Whether to use the default transition animation
   */
  void PushModal(View view, bool animated = true);

  /**
   * @brief Pops the top regular navigation view.
   *
   * @param[in] animated Whether to use the default transition animation
   * @return The popped view, or an uninitialized handle
   */
  View Pop(bool animated = true);

  /**
   * @brief Pops the top modal view.
   *
   * @param[in] animated Whether to use the default transition animation
   * @return The popped modal view, or an uninitialized handle
   */
  View PopModal(bool animated = true);

  /**
   * @brief Inserts a regular navigation view immediately before another view.
   *
   * @param[in] view The view to insert
   * @param[in] before The existing navigation view to insert before
   */
  void InsertBefore(View view, View before);

  /**
   * @brief Removes a view from either stack.
   *
   * @param[in] view The view to remove
   */
  void Remove(View view);

  /**
   * @brief Removes all managed views.
   */
  void Clear();

  /**
   * @brief Navigates back from the current top view.
   *
   * Modal views are popped first. Regular navigation views are popped until the
   * root view remains.
   *
   * @return True if back navigation was handled
   */
  bool NavigateBack();

public: // Setters for chaining
  // @CHAIN_START(Navigator, View)
  // @CHAIN_END

public: // Not intended for application developers
  /// @cond internal
  DALI_INTERNAL          Navigator(Internal::NavigatorImpl& implementation);
  explicit DALI_INTERNAL Navigator(Dali::Internal::CustomActor* internal);
  /// @endcond

public:
  DALI_UI_CHAIN_VIEW_METHODS(Navigator)
};

} // namespace Ui

} // namespace Dali
