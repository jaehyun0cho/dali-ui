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

#include <stdlib.h>
#include <dali.h>
#include <dali-ui-components/public-api/navigator.h>
#include <dali-ui-foundation/dali-ui-foundation.h>
#include <dali-ui-test-suite-utils.h>
#include <stdexcept>

using namespace Dali;
using namespace Dali::Ui;

void utc_dali_navigator_startup(void)
{
  test_return_value = TET_UNDEF;
}

void utc_dali_navigator_cleanup(void)
{
  test_return_value = TET_PASS;
}

int UtcDaliNavigatorConstructorP(void)
{
  UiTestApplication application;
  Navigator navigator;
  DALI_TEST_CHECK(!navigator);
  END_TEST;
}

int UtcDaliNavigatorNewP(void)
{
  UiTestApplication application;
  Navigator navigator = Navigator::New();
  DALI_TEST_CHECK(navigator);
  DALI_TEST_EQUALS(0u, navigator.GetNavigationStackCount(), TEST_LOCATION);
  DALI_TEST_EQUALS(0u, navigator.GetModalStackCount(), TEST_LOCATION);
  DALI_TEST_CHECK(!navigator.GetCurrentView());
  END_TEST;
}

int UtcDaliNavigatorCopyAndMoveP(void)
{
  UiTestApplication application;
  Navigator navigator = Navigator::New();
  Navigator copy(navigator);
  DALI_TEST_CHECK(copy);
  DALI_TEST_CHECK(copy == navigator);

  Navigator moved(std::move(copy));
  DALI_TEST_CHECK(moved);
  DALI_TEST_CHECK(moved == navigator);
  DALI_TEST_CHECK(!copy);
  END_TEST;
}

int UtcDaliNavigatorAssignmentP(void)
{
  UiTestApplication application;
  Navigator navigator = Navigator::New();
  Navigator copy;
  copy = navigator;
  DALI_TEST_CHECK(copy);
  DALI_TEST_CHECK(copy == navigator);

  Navigator moved;
  moved = std::move(copy);
  DALI_TEST_CHECK(moved);
  DALI_TEST_CHECK(moved == navigator);
  DALI_TEST_CHECK(!copy);
  END_TEST;
}

int UtcDaliNavigatorDownCastP(void)
{
  UiTestApplication application;
  Navigator navigator = Navigator::New();
  BaseHandle object(navigator);
  Navigator navigator2 = Navigator::DownCast(object);
  Navigator navigator3 = DownCast<Navigator>(object);
  DALI_TEST_CHECK(navigator2);
  DALI_TEST_CHECK(navigator3);
  END_TEST;
}

int UtcDaliNavigatorDownCastN(void)
{
  UiTestApplication application;
  BaseHandle object;
  Navigator navigator2 = Navigator::DownCast(object);
  Navigator navigator3 = DownCast<Navigator>(object);
  DALI_TEST_CHECK(!navigator2);
  DALI_TEST_CHECK(!navigator3);
  END_TEST;
}

int UtcDaliNavigatorPushPopP(void)
{
  UiTestApplication application;
  Navigator navigator = Navigator::New();
  View root           = View::New();
  View second         = View::New();

  navigator.Push(root, false);
  DALI_TEST_EQUALS(1u, navigator.GetNavigationStackCount(), TEST_LOCATION);
  DALI_TEST_EQUALS(root, navigator.GetNavigationStackAt(0u), TEST_LOCATION);
  DALI_TEST_EQUALS(root, navigator.GetCurrentView(), TEST_LOCATION);

  navigator.Push(second, false);
  DALI_TEST_EQUALS(2u, navigator.GetNavigationStackCount(), TEST_LOCATION);
  DALI_TEST_EQUALS(second, navigator.GetCurrentView(), TEST_LOCATION);
  DALI_TEST_CHECK(!root.IsVisible());

  View popped = navigator.Pop(false);
  DALI_TEST_EQUALS(second, popped, TEST_LOCATION);
  DALI_TEST_EQUALS(1u, navigator.GetNavigationStackCount(), TEST_LOCATION);
  DALI_TEST_EQUALS(root, navigator.GetCurrentView(), TEST_LOCATION);
  DALI_TEST_CHECK(root.IsVisible());
  END_TEST;
}

int UtcDaliNavigatorPushModalPopModalP(void)
{
  UiTestApplication application;
  Navigator navigator = Navigator::New();
  View root           = View::New();
  View modal          = View::New();

  navigator.Push(root, false);
  navigator.PushModal(modal, false);

  DALI_TEST_EQUALS(1u, navigator.GetNavigationStackCount(), TEST_LOCATION);
  DALI_TEST_EQUALS(1u, navigator.GetModalStackCount(), TEST_LOCATION);
  DALI_TEST_EQUALS(modal, navigator.GetModalStackAt(0u), TEST_LOCATION);
  DALI_TEST_EQUALS(modal, navigator.GetCurrentView(), TEST_LOCATION);
  DALI_TEST_CHECK(!root.IsEnabled());

  View popped = navigator.PopModal(false);
  DALI_TEST_EQUALS(modal, popped, TEST_LOCATION);
  DALI_TEST_EQUALS(0u, navigator.GetModalStackCount(), TEST_LOCATION);
  DALI_TEST_EQUALS(root, navigator.GetCurrentView(), TEST_LOCATION);
  DALI_TEST_CHECK(root.IsEnabled());
  END_TEST;
}

int UtcDaliNavigatorInsertBeforeP(void)
{
  UiTestApplication application;
  Navigator navigator = Navigator::New();
  View root           = View::New();
  View second         = View::New();
  View inserted       = View::New();

  navigator.Push(root, false);
  navigator.Push(second, false);
  navigator.InsertBefore(inserted, second);

  DALI_TEST_EQUALS(3u, navigator.GetNavigationStackCount(), TEST_LOCATION);
  DALI_TEST_EQUALS(root, navigator.GetNavigationStackAt(0u), TEST_LOCATION);
  DALI_TEST_EQUALS(inserted, navigator.GetNavigationStackAt(1u), TEST_LOCATION);
  DALI_TEST_EQUALS(second, navigator.GetNavigationStackAt(2u), TEST_LOCATION);
  DALI_TEST_EQUALS(second, navigator.GetCurrentView(), TEST_LOCATION);
  DALI_TEST_CHECK(!inserted.IsVisible());
  END_TEST;
}

int UtcDaliNavigatorRemoveP(void)
{
  UiTestApplication application;
  Navigator navigator = Navigator::New();
  View root           = View::New();
  View middle         = View::New();
  View second         = View::New();

  navigator.Push(root, false);
  navigator.Push(middle, false);
  navigator.Push(second, false);
  navigator.Remove(middle);

  DALI_TEST_EQUALS(2u, navigator.GetNavigationStackCount(), TEST_LOCATION);
  DALI_TEST_EQUALS(root, navigator.GetNavigationStackAt(0u), TEST_LOCATION);
  DALI_TEST_EQUALS(second, navigator.GetNavigationStackAt(1u), TEST_LOCATION);
  DALI_TEST_EQUALS(second, navigator.GetCurrentView(), TEST_LOCATION);

  navigator.Remove(second);
  DALI_TEST_EQUALS(1u, navigator.GetNavigationStackCount(), TEST_LOCATION);
  DALI_TEST_EQUALS(root, navigator.GetCurrentView(), TEST_LOCATION);
  END_TEST;
}

int UtcDaliNavigatorClearP(void)
{
  UiTestApplication application;
  Navigator navigator = Navigator::New();
  navigator.Push(View::New(), false);
  navigator.Push(View::New(), false);
  navigator.PushModal(View::New(), false);

  navigator.Clear();
  DALI_TEST_EQUALS(0u, navigator.GetNavigationStackCount(), TEST_LOCATION);
  DALI_TEST_EQUALS(0u, navigator.GetModalStackCount(), TEST_LOCATION);
  DALI_TEST_CHECK(!navigator.GetCurrentView());
  END_TEST;
}

int UtcDaliNavigatorNavigateBackP(void)
{
  UiTestApplication application;
  Navigator navigator = Navigator::New();
  View root           = View::New();
  View second         = View::New();
  View modal          = View::New();

  navigator.Push(root, false);
  DALI_TEST_CHECK(!navigator.NavigateBack());

  navigator.Push(second, false);
  navigator.PushModal(modal, false);

  DALI_TEST_CHECK(navigator.NavigateBack());
  DALI_TEST_EQUALS(0u, navigator.GetModalStackCount(), TEST_LOCATION);
  DALI_TEST_EQUALS(second, navigator.GetCurrentView(), TEST_LOCATION);

  DALI_TEST_CHECK(navigator.NavigateBack());
  DALI_TEST_EQUALS(root, navigator.GetCurrentView(), TEST_LOCATION);
  DALI_TEST_CHECK(!navigator.NavigateBack());
  END_TEST;
}

int UtcDaliNavigatorInvalidOperationsN(void)
{
  UiTestApplication application;
  Navigator navigator = Navigator::New();
  View view           = View::New();
  View missing        = View::New();

  DALI_TEST_THROWS(navigator.Push(View(), false), std::invalid_argument);

  navigator.Push(view, false);
  DALI_TEST_THROWS(navigator.Push(view, false), std::invalid_argument);
  DALI_TEST_THROWS(navigator.Remove(missing), std::invalid_argument);
  DALI_TEST_THROWS(navigator.InsertBefore(missing, View::New()), std::invalid_argument);
  END_TEST;
}
