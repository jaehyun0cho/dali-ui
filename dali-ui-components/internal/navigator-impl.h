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
#include <dali-ui-foundation/public-api/view-impl.h>
#include <dali/public-api/animation/animation.h>
#include <utility>
#include <vector>

// INTERNAL INCLUDES
#include <dali-ui-components/public-api/navigator.h>

namespace Dali
{

namespace Ui
{

namespace Internal
{

/**
 * @brief Internal implementation for Navigator.
 */
class NavigatorImpl : public ViewImpl
{
public:
  static Ui::Navigator New();

protected:
  virtual ~NavigatorImpl();

public:
  uint32_t GetNavigationStackCount() const;
  uint32_t GetModalStackCount() const;
  Ui::View GetNavigationStackAt(uint32_t index) const;
  Ui::View GetModalStackAt(uint32_t index) const;
  Ui::View GetCurrentView() const;

  void     Push(Ui::View view, bool animated);
  void     PushModal(Ui::View view, bool animated);
  Ui::View Pop(bool animated);
  Ui::View PopModal(bool animated);
  void     InsertBefore(Ui::View view, Ui::View before);
  void     Remove(Ui::View view);
  void     Clear();
  bool     NavigateBack();

protected:
  NavigatorImpl();
  void OnInitialize() override;

private:
  NavigatorImpl(const NavigatorImpl&)            = delete;
  NavigatorImpl(NavigatorImpl&&)                 = delete;
  NavigatorImpl& operator=(const NavigatorImpl&) = delete;
  NavigatorImpl& operator=(NavigatorImpl&&)      = delete;

  using Stack = std::vector<Ui::View>;

  bool                  Contains(Ui::View view) const;
  bool                  Contains(const Stack& stack, Ui::View view) const;
  Stack::iterator       Find(Stack& stack, Ui::View view);
  Stack::const_iterator Find(const Stack& stack, Ui::View view) const;
  Ui::View              GetTopView(const Stack& stack) const;
  void                  ThrowIfInvalidOrContained(Ui::View view) const;
  void                  ThrowIfMissing(Ui::View view) const;
  void                  SaveNavigationProperties(Ui::View view);
  void                  LoadNavigationProperties(Ui::View view);
  void                  RemoveInternal(Ui::View view);
  void                  SetHiddenBelowTop(Ui::View view, bool hidden);
  void                  PlayDefaultTransition(Ui::View appearing, Ui::View disappearing, bool animated, bool modal);
  void                  StopCurrentAnimation();

private:
  struct NavigationProperties
  {
    bool enabled{true};
    bool focusable{false};
    bool descendantFocusBlocked{false};
  };

  Stack                                                  mNavigationStack;
  Stack                                                  mModalStack;
  std::vector<std::pair<Ui::View, NavigationProperties>> mSavedProperties;
  Dali::Animation                                        mCurrentAnimation;
};

} // namespace Internal

inline Internal::NavigatorImpl& GetImpl(Ui::Navigator& navigator)
{
  DALI_ASSERT_ALWAYS(navigator);

  Dali::RefObject& handle = navigator.GetImplementation();

  return static_cast<Internal::NavigatorImpl&>(handle);
}

inline const Internal::NavigatorImpl& GetImpl(const Ui::Navigator& navigator)
{
  DALI_ASSERT_ALWAYS(navigator);

  const Dali::RefObject& handle = navigator.GetImplementation();

  return static_cast<const Internal::NavigatorImpl&>(handle);
}

} // namespace Ui

} // namespace Dali
