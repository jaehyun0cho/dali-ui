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
#include <iostream>

#include <dali-ui-foundation/dali-ui-foundation.h>
#include <dali-ui-foundation/public-api/group-selectable-trait.h>
#include <dali-ui-foundation/public-api/selectable-group.h>
#include <dali-ui-foundation/public-api/view-accessibility-enums.h>
#include <dali-ui-test-suite-utils.h>
#include <dali.h>
#include <test-gesture-generator.h>

using namespace Dali;
using namespace Dali::Ui;

namespace
{

const int CHECKED_MASK = 1 << static_cast<int>(AccessibilityState::CHECKED);

bool IsChecked(View view)
{
  return (view.GetProperty<int>(View::Property::ACCESSIBILITY_STATES) & CHECKED_MASK) != 0;
}

View CreateView(UiTestApplication& application, float x = 0.0f)
{
  View view = View::New();
  view.SetRequestedWidth(100.0f);
  view.SetRequestedHeight(100.0f);
  view.SetProperty(Actor::Property::PIVOT, Pivot::TOP_LEFT);
  view.SetProperty(Actor::Property::PARENT_ORIGIN, ParentOrigin::TOP_LEFT);
  view.SetProperty(Actor::Property::POSITION, Vector3(x, 0.0f, 0.0f));
  application.GetScene().Add(view);
  application.SendNotification();
  application.Render();
  return view;
}

} // namespace

void utc_dali_groupselectabletrait_startup(void)
{
  test_return_value = TET_UNDEF;
}

void utc_dali_groupselectabletrait_cleanup(void)
{
  test_return_value = TET_PASS;
}

// ============================================================================
// Construction / Handle
// ============================================================================

int UtcDaliGroupSelectableTraitDefaultConstructorN(void)
{
  UiTestApplication    application;
  GroupSelectableTrait trait;
  DALI_TEST_CHECK(!trait);
  END_TEST;
}

int UtcDaliGroupSelectableTraitDownCastFromMemberP(void)
{
  UiTestApplication application;
  SelectableGroup   group = SelectableGroup::New();
  View              view  = CreateView(application);
  group.Add(view);

  GroupSelectableTrait trait = GroupSelectableTrait::DownCast(view.AsSelectable());
  DALI_TEST_CHECK(trait);
  END_TEST;
}

int UtcDaliGroupSelectableTraitDownCastFromPlainN(void)
{
  UiTestApplication application;
  View              view = CreateView(application);

  // A plain selectable trait is not a GroupSelectableTrait.
  SelectableTrait      plain = view.AsSelectable();
  GroupSelectableTrait trait = GroupSelectableTrait::DownCast(plain);
  DALI_TEST_CHECK(!trait);
  END_TEST;
}

int UtcDaliGroupSelectableTraitDownCastN(void)
{
  UiTestApplication    application;
  BaseHandle           handle;
  GroupSelectableTrait trait = GroupSelectableTrait::DownCast(handle);
  DALI_TEST_CHECK(!trait);
  END_TEST;
}

int UtcDaliGroupSelectableTraitCopyConstructorP(void)
{
  UiTestApplication application;
  SelectableGroup   group = SelectableGroup::New();
  View              view  = CreateView(application);
  group.Add(view);

  GroupSelectableTrait trait = GroupSelectableTrait::DownCast(view.AsSelectable());
  GroupSelectableTrait copy(trait);
  DALI_TEST_CHECK(copy);
  DALI_TEST_CHECK(copy == trait);
  END_TEST;
}

int UtcDaliGroupSelectableTraitGetGroupP(void)
{
  UiTestApplication application;
  SelectableGroup   group = SelectableGroup::New();
  View              view  = CreateView(application);
  group.Add(view);

  GroupSelectableTrait trait = GroupSelectableTrait::DownCast(view.AsSelectable());
  DALI_TEST_CHECK(trait.GetGroup() == group);
  END_TEST;
}

// Note: GroupSelectableTrait::New() taking no argument is intentionally a compile
// error (it hides the inherited SelectableTrait::New()). Instances are created
// only through SelectableGroup::Add(). This cannot be exercised at runtime.

// ============================================================================
// Inherited SelectableTrait behaviour
// ============================================================================

int UtcDaliGroupSelectableTraitIsSelectableP(void)
{
  UiTestApplication application;
  SelectableGroup   group = SelectableGroup::New();
  View              view  = CreateView(application);
  group.Add(view);

  // A GroupSelectableTrait IS a SelectableTrait.
  SelectableTrait selectable = view.AsSelectable();
  DALI_TEST_CHECK(selectable);
  DALI_TEST_CHECK(!selectable.IsSelected());

  group.SelectMember(view);
  DALI_TEST_CHECK(selectable.IsSelected());
  END_TEST;
}

int UtcDaliGroupSelectableTraitSelectionChangedSignalP(void)
{
  UiTestApplication application;
  SelectableGroup   group = SelectableGroup::New();
  View              view  = CreateView(application);
  group.Add(view);

  bool called   = false;
  bool selected = false;
  view.AsSelectable().SelectionChangedSignal().Connect(
    &application, [&called, &selected](View v, bool s, InputEvent e)
  {
    called   = true;
    selected = s;
  });

  group.SelectMember(view);
  DALI_TEST_CHECK(called);
  DALI_TEST_CHECK(selected);
  END_TEST;
}

// ============================================================================
// Winner deselect contract (SetSelected(false) is a no-op for the winner)
// ============================================================================

int UtcDaliGroupSelectableTraitSetSelectedFalseNoOpP(void)
{
  UiTestApplication application;
  SelectableGroup   group = SelectableGroup::New();
  View              view  = CreateView(application);
  group.Add(view);
  group.SelectMember(view);
  DALI_TEST_CHECK(view.AsSelectable().IsSelected());

  // The grouped winner cannot be deselected through SetSelected(false).
  view.AsSelectable().SetSelected(false);
  DALI_TEST_CHECK(view.AsSelectable().IsSelected());
  DALI_TEST_CHECK(IsChecked(view));
  END_TEST;
}

int UtcDaliGroupSelectableTraitCheckedSeedFalseP(void)
{
  UiTestApplication application;
  SelectableGroup   group = SelectableGroup::New();
  View              view  = CreateView(application);
  group.Add(view);

  // Before any selection, CHECKED is seeded false.
  DALI_TEST_CHECK(!IsChecked(view));
  END_TEST;
}

// ============================================================================
// Toggle-by-click (radio semantics)
// ============================================================================

int UtcDaliGroupSelectableTraitClickSelectsP(void)
{
  UiTestApplication application;
  SelectableGroup   group = SelectableGroup::New();
  View              view  = CreateView(application);
  group.Add(view);

  application.SendNotification();
  application.Render();

  // Toggle-by-click is enabled by default; the first tap selects.
  TestGenerateTap(application, 50.0f, 50.0f, 100);
  DALI_TEST_CHECK(view.AsSelectable().IsSelected());
  DALI_TEST_CHECK(IsChecked(view));
  END_TEST;
}

int UtcDaliGroupSelectableTraitReclickIsNoOpP(void)
{
  UiTestApplication application;
  SelectableGroup   group = SelectableGroup::New();
  View              view  = CreateView(application);
  group.Add(view);

  application.SendNotification();
  application.Render();

  // First tap selects.
  TestGenerateTap(application, 50.0f, 50.0f, 100);
  DALI_TEST_CHECK(view.AsSelectable().IsSelected());

  // Re-clicking the selected radio is a no-op (stays selected).
  TestGenerateTap(application, 50.0f, 50.0f, 300);
  DALI_TEST_CHECK(view.AsSelectable().IsSelected());
  DALI_TEST_CHECK(IsChecked(view));
  END_TEST;
}

int UtcDaliGroupSelectableTraitClickSwapP(void)
{
  UiTestApplication application;
  SelectableGroup   group = SelectableGroup::New();
  // Two overlapping members; b is added later, so it is the topmost sibling and
  // receives the tap. This drives a click-based swap deterministically.
  View a = CreateView(application);
  View b = CreateView(application);
  group.Add(a);
  group.Add(b);

  // Select A programmatically (hit-testing is not used by SelectMember).
  group.SelectMember(a);
  application.SendNotification();
  application.Render();
  DALI_TEST_CHECK(a.AsSelectable().IsSelected());
  DALI_TEST_CHECK(!b.AsSelectable().IsSelected());

  // Tap hits B (topmost): click-driven swap -> B selected, A deselected.
  TestGenerateTap(application, 50.0f, 50.0f, 100);
  DALI_TEST_CHECK(!a.AsSelectable().IsSelected());
  DALI_TEST_CHECK(b.AsSelectable().IsSelected());
  DALI_TEST_CHECK(!IsChecked(a));
  DALI_TEST_CHECK(IsChecked(b));
  END_TEST;
}
