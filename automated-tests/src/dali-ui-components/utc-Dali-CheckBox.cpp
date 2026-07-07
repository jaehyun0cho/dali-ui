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

#include <dali-ui-test-suite-utils.h>
#include <dali-ui-components/dali-ui-components.h>
#include <dali-ui-foundation/public-api/views/view.h>
#include <dali-ui-foundation/public-api/views/view-accessibility-types.h>

using namespace Dali;
using namespace Dali::Ui;

void utc_dali_check_box_startup(void)
{
  test_return_value = TET_UNDEF;
}
void utc_dali_check_box_cleanup(void)
{
  test_return_value = TET_PASS;
}

namespace
{
// Observes SelectionChangedSignal by reference.
struct SelectionSpy
{
  SelectionSpy(int& count, bool& last)
  : mCount(count), mLast(last)
  {
  }
  void operator()(View, bool selected, InputEvent)
  {
    ++mCount;
    mLast = selected;
  }
  int&  mCount;
  bool& mLast;
};
} // namespace

int UtcDaliCheckBoxNewP(void)
{
  UiTestApplication application(Components::UiConfig::New());
  CheckBox          cb = CheckBox::New();
  DALI_TEST_CHECK(cb);
  DALI_TEST_CHECK(SelectableView::DownCast(cb)); // is-a selectable
  DALI_TEST_CHECK(!cb.IsSelected());             // default unchecked
  END_TEST;
}

int UtcDaliCheckBoxCopyMoveDownCast(void)
{
  UiTestApplication application(Components::UiConfig::New());
  CheckBox          cb = CheckBox::New();

  CheckBox copy(cb);
  DALI_TEST_CHECK(copy == cb);
  CheckBox moved(std::move(copy));
  DALI_TEST_CHECK(moved == cb);

  BaseHandle handle(cb);
  DALI_TEST_CHECK(CheckBox::DownCast(handle));
  DALI_TEST_CHECK(!CheckBox::DownCast(BaseHandle())); // empty for unrelated
  END_TEST;
}

int UtcDaliCheckBoxRoleIsCheckBox(void)
{
  UiTestApplication application(Components::UiConfig::New());
  CheckBox          cb    = CheckBox::New();
  Actor             actor = cb;
  int32_t           role  = actor.GetProperty<int32_t>(Ui::View::Property::ACCESSIBILITY_ROLE);
  DALI_TEST_EQUALS(role, static_cast<int32_t>(Accessibility::Role::CHECK_BOX), TEST_LOCATION);
  END_TEST;
}

int UtcDaliCheckBoxSelectTogglesCheckedBitPreservesEnabled(void)
{
  UiTestApplication application(Components::UiConfig::New());
  CheckBox          cb = CheckBox::New();

  // Capture the ENABLED bit up front, then assert the CHECKED toggle preserves it
  // (AddAccessibilityState/RemoveAccessibilityState are additive/subtractive, not a
  // whole-bitset replacement).
  const bool enabledBefore = cb.HasAccessibilityState(Accessibility::State::ENABLED);

  cb.SetSelected(true);
  DALI_TEST_CHECK(cb.HasAccessibilityState(Accessibility::State::CHECKED));
  DALI_TEST_EQUALS(cb.HasAccessibilityState(Accessibility::State::ENABLED), enabledBefore, TEST_LOCATION);

  cb.SetSelected(false);
  DALI_TEST_CHECK(!cb.HasAccessibilityState(Accessibility::State::CHECKED));
  DALI_TEST_EQUALS(cb.HasAccessibilityState(Accessibility::State::ENABLED), enabledBefore, TEST_LOCATION);
  END_TEST;
}

int UtcDaliCheckBoxSelectionChangedFiresOncePerChange(void)
{
  UiTestApplication application(Components::UiConfig::New());
  CheckBox          cb = CheckBox::New();

  int  count = 0;
  bool last  = false;
  cb.SelectionChangedSignal().Connect(&application, SelectionSpy(count, last));

  cb.SetSelected(true);
  DALI_TEST_EQUALS(count, 1, TEST_LOCATION);
  DALI_TEST_EQUALS(last, true, TEST_LOCATION);

  cb.SetSelected(true); // same value -> no emit (short-circuit)
  DALI_TEST_EQUALS(count, 1, TEST_LOCATION);

  cb.SetSelected(false);
  DALI_TEST_EQUALS(count, 2, TEST_LOCATION);
  DALI_TEST_EQUALS(last, false, TEST_LOCATION);
  END_TEST;
}

int UtcDaliCheckBoxTextBoxOnlyAndLabel(void)
{
  UiTestApplication application(Components::UiConfig::New());

  CheckBox boxOnly = CheckBox::New();
  DALI_TEST_EQUALS(boxOnly.GetText(), std::string(""), TEST_LOCATION);

  CheckBox labelled = CheckBox::New("Agree");
  DALI_TEST_EQUALS(labelled.GetText(), std::string("Agree"), TEST_LOCATION);

  labelled.SetText("");
  DALI_TEST_EQUALS(labelled.GetText(), std::string(""), TEST_LOCATION);
  END_TEST;
}

int UtcDaliCheckBoxSelectionAnimationMode(void)
{
  UiTestApplication application(Components::UiConfig::New());
  CheckBox          cb = CheckBox::New();

  DALI_TEST_EQUALS(static_cast<int>(cb.GetSelectionAnimationMode()),
                   static_cast<int>(SelectionAnimationMode::AUTO), TEST_LOCATION); // default

  cb.SetSelectionAnimationMode(SelectionAnimationMode::DISABLED);
  DALI_TEST_EQUALS(static_cast<int>(cb.GetSelectionAnimationMode()),
                   static_cast<int>(SelectionAnimationMode::DISABLED), TEST_LOCATION);

  // Off-scene programmatic changes must snap without crashing regardless of mode.
  cb.SetSelectionAnimationMode(SelectionAnimationMode::ENABLED);
  cb.SetSelected(true);
  DALI_TEST_CHECK(cb.IsSelected());
  END_TEST;
}

int UtcDaliCheckBoxStyleBuilderRoundTrip(void)
{
  UiTestApplication application(Components::UiConfig::New());

  CheckBoxStyle style = CheckBoxStyle::Builder()
                          .SetBoxSize(36.0f)
                          .SetLabelGap(8.0f)
                          .SetMinimumWidth(48.0f)
                          .SetIconUrl("i_check_box.json")
                          .Build();

  DALI_TEST_EQUALS(style.GetBoxSize(), 36.0f, TEST_LOCATION);
  DALI_TEST_EQUALS(style.GetLabelGap(), 8.0f, TEST_LOCATION);
  DALI_TEST_EQUALS(style.GetMinimumWidth(), 48.0f, TEST_LOCATION);
  DALI_TEST_EQUALS(style.GetIconUrl(), std::string("i_check_box.json"), TEST_LOCATION);

  CheckBox cb = CheckBox::New(style);
  DALI_TEST_CHECK(cb);
  DALI_TEST_EQUALS(cb.GetMinimumWidth(), 48.0f, TEST_LOCATION); // inherited from View
  END_TEST;
}

int UtcDaliCheckBoxDisabledState(void)
{
  UiTestApplication application(Components::UiConfig::New());
  CheckBox          cb = CheckBox::New();

  // §1.4.3: disabling uses the inherited View::SetEnabled(false), which blocks user
  // toggles via disabled interaction (DALi does NOT auto-deselect on disable).
  cb.SetEnabled(false);
  DALI_TEST_CHECK(!cb.IsEnabled());

  // Programmatic SetSelected is unaffected by disable.
  cb.SetSelected(true);
  DALI_TEST_CHECK(cb.IsSelected());

  // Note: asserting that a user CLICK is suppressed while disabled requires touch/key
  // injection (see §7 notes); the deterministic checks above cover the disable API.
  END_TEST;
}
