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

#include <dali.h>
#include <dali-ui-foundation/dali-ui-foundation.h>
#include <dali-ui-foundation/public-api/group-selectable-trait.h>
#include <dali-ui-foundation/public-api/selection-group.h>
#include <dali-ui-foundation/public-api/view-accessibility-enums.h>
#include <dali-ui-test-suite-utils.h>
#include <test-gesture-generator.h>

using namespace Dali;
using namespace Dali::Ui;

namespace
{

// CHECKED bit mask within the ACCESSIBILITY_STATES bitset. CHECKED is index 2 of
// the AccessibilityState enum (view-accessibility-enums.h:31), so the mask is
// 1u << 2. ENABLED (index 0) is set by default, which is why CHECKED must be
// written via read-modify-write.
constexpr uint32_t CHECKED_MASK = 1u << static_cast<uint32_t>(AccessibilityState::CHECKED);

bool IsRadioButton(View view)
{
  return view.GetProperty<int>(View::Property::ACCESSIBILITY_ROLE) ==
         static_cast<int>(AccessibilityRole::RADIO_BUTTON);
}

bool IsChecked(View view)
{
  const uint32_t states = static_cast<uint32_t>(view.GetProperty<int>(View::Property::ACCESSIBILITY_STATES));
  return (states & CHECKED_MASK) != 0u;
}

bool IsEnabledStateSet(View view)
{
  const uint32_t enabledMask = 1u << static_cast<uint32_t>(AccessibilityState::ENABLED);
  const uint32_t states      = static_cast<uint32_t>(view.GetProperty<int>(View::Property::ACCESSIBILITY_STATES));
  return (states & enabledMask) != 0u;
}

// Creates a View sized for tapping, but does NOT add it to a scene.
View CreateView()
{
  View view = View::New();
  view.SetRequestedWidth(100.0f);
  view.SetRequestedHeight(100.0f);
  view.SetProperty(Actor::Property::PIVOT, Pivot::TOP_LEFT);
  view.SetProperty(Actor::Property::PARENT_ORIGIN, ParentOrigin::TOP_LEFT);
  return view;
}

// Creates a View on the scene at a given position, ready for tapping.
View CreateSceneView(UiTestApplication& application, float x = 0.0f, float y = 0.0f)
{
  View view = CreateView();
  view.SetProperty(Actor::Property::POSITION, Vector3(x, y, 0.0f));
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
// Construction / Handle  (mirrors utc-Dali-SelectableTrait.cpp New/DownCast)
// ============================================================================

int UtcDaliGroupSelectableTraitNewP(void)
{
  UiTestApplication    application;
  GroupSelectableTrait trait = GroupSelectableTrait::New();
  DALI_TEST_CHECK(trait);
  END_TEST;
}

int UtcDaliGroupSelectableTraitDefaultConstructorN(void)
{
  UiTestApplication    application;
  GroupSelectableTrait trait;
  DALI_TEST_CHECK(!trait);
  END_TEST;
}

int UtcDaliGroupSelectableTraitCopyConstructorP(void)
{
  UiTestApplication    application;
  GroupSelectableTrait trait = GroupSelectableTrait::New();
  GroupSelectableTrait copy(trait);
  DALI_TEST_CHECK(copy);
  DALI_TEST_CHECK(copy == trait);
  END_TEST;
}

int UtcDaliGroupSelectableTraitDownCastP(void)
{
  UiTestApplication    application;
  GroupSelectableTrait trait = GroupSelectableTrait::New();
  BaseHandle           handle(trait);
  GroupSelectableTrait downcast = GroupSelectableTrait::DownCast(handle);
  DALI_TEST_CHECK(downcast);
  END_TEST;
}

int UtcDaliGroupSelectableTraitDownCastN(void)
{
  UiTestApplication    application;
  BaseHandle           handle;
  GroupSelectableTrait downcast = GroupSelectableTrait::DownCast(handle);
  DALI_TEST_CHECK(!downcast);
  END_TEST;
}

// DownCast of a plain SelectableTrait (which is NOT group-selectable) must fail,
// because DownCast gates on the presence of the group sub-impl (mirrors the
// SelectableTrait DownCast gating on its own sub-impl).
int UtcDaliGroupSelectableTraitDownCastFromSelectableN(void)
{
  UiTestApplication application;
  View              view = View::New();

  SelectableTrait      selectable = view.AsSelectable();
  GroupSelectableTrait downcast   = GroupSelectableTrait::DownCast(selectable);
  DALI_TEST_CHECK(!downcast);
  END_TEST;
}

// ============================================================================
// Handle inheritance chain: GroupSelectable -> Selectable -> Interactive
// ============================================================================

int UtcDaliGroupSelectableTraitHandleChainP(void)
{
  UiTestApplication    application;
  GroupSelectableTrait trait = GroupSelectableTrait::New();

  // GroupSelectableTrait IS-A SelectableTrait IS-A InteractiveTrait by static type.
  SelectableTrait  asSelectable  = trait;
  InteractiveTrait asInteractive = trait;
  DALI_TEST_CHECK(asSelectable);
  DALI_TEST_CHECK(asInteractive);

  // Same underlying object across the whole chain.
  DALI_TEST_CHECK(asSelectable == trait);
  DALI_TEST_CHECK(asInteractive == trait);

  // Selectable APIs are reachable through the group handle.
  DALI_TEST_CHECK(!trait.IsSelected());
  // Interactive APIs are reachable through the group handle.
  trait.ClickedSignal();
  END_TEST;
}

// ============================================================================
// AsGroupSelectable (View integration) + implies-chain made structural
// ============================================================================

int UtcDaliViewAsGroupSelectableP(void)
{
  UiTestApplication application;
  View              view = View::New();

  GroupSelectableTrait result = view.AsGroupSelectable();
  DALI_TEST_CHECK(result);

  DALI_TEST_CHECK(view.IsGroupSelectable());
  END_TEST;
}

// "GroupSelectable implies Selectable implies Interactive" is structural:
// AsGroupSelectable must ensure the whole chain.
int UtcDaliViewAsGroupSelectableImpliesSelectableAndInteractiveP(void)
{
  UiTestApplication application;
  View              view = View::New();

  view.AsGroupSelectable();

  DALI_TEST_CHECK(view.IsGroupSelectable());
  DALI_TEST_CHECK(view.IsSelectable());
  DALI_TEST_CHECK(view.IsInteractive());
  END_TEST;
}

int UtcDaliViewIsGroupSelectableWithoutAttachN(void)
{
  UiTestApplication application;
  View              view = View::New();

  DALI_TEST_CHECK(!view.IsGroupSelectable());
  END_TEST;
}

// A plain selectable View is not group-selectable (cost-free for non-group Views).
int UtcDaliViewSelectableIsNotGroupSelectableN(void)
{
  UiTestApplication application;
  View              view = View::New();

  view.AsSelectable();

  DALI_TEST_CHECK(view.IsSelectable());
  DALI_TEST_CHECK(!view.IsGroupSelectable());
  END_TEST;
}

int UtcDaliViewAsGroupSelectableIdempotentP(void)
{
  UiTestApplication application;
  View              view = View::New();

  view.AsGroupSelectable();
  GroupSelectableTrait first = view.AsGroupSelectable();

  view.AsGroupSelectable();
  GroupSelectableTrait second = view.AsGroupSelectable();

  DALI_TEST_CHECK(first == second);
  END_TEST;
}

// Upgrading an existing selectable View to group-selectable reuses the same
// container (one CORE_INTERACTION_TRAITS slot).
int UtcDaliViewSelectableThenGroupSelectableReusesContainerP(void)
{
  UiTestApplication application;
  View              view = View::New();

  SelectableTrait selectable = view.AsSelectable();

  GroupSelectableTrait groupSelectable = view.AsGroupSelectable();
  DALI_TEST_CHECK(groupSelectable);
  DALI_TEST_CHECK(view.IsSelectable());
  DALI_TEST_CHECK(view.IsGroupSelectable());

  // The group handle and the plain selectable handle refer to the same object.
  DALI_TEST_CHECK(static_cast<BaseHandle>(groupSelectable) == static_cast<BaseHandle>(selectable));
  END_TEST;
}

// ============================================================================
// GetGroup() (read-only on the trait; empty before joining a group)
// ============================================================================

int UtcDaliGroupSelectableTraitGetGroupEmptyByDefaultP(void)
{
  UiTestApplication application;
  View              view = View::New();

  GroupSelectableTrait trait = view.AsGroupSelectable();
  DALI_TEST_CHECK(!trait.GetGroup());
  END_TEST;
}

int UtcDaliGroupSelectableTraitGetGroupAfterAddP(void)
{
  UiTestApplication application;
  View              view = View::New();

  SelectionGroup group = SelectionGroup::New();
  group.Add(view);

  GroupSelectableTrait trait = view.AsGroupSelectable();
  SelectionGroup       got   = trait.GetGroup();
  DALI_TEST_CHECK(got);
  DALI_TEST_CHECK(got == group);
  END_TEST;
}

// ============================================================================
// No-op reclick (true radio): tapping the already-selected member keeps it.
// ============================================================================

int UtcDaliGroupSelectableTraitReclickNoOpP(void)
{
  UiTestApplication application;
  View              view = CreateSceneView(application, 0.0f, 0.0f);

  SelectionGroup group = SelectionGroup::New();
  group.Add(view);

  GroupSelectableTrait trait      = view.AsGroupSelectable();
  SelectableTrait      selectable = view.AsSelectable();

  // First tap selects.
  TestGenerateTap(application, 50.0f, 50.0f, 100);
  DALI_TEST_CHECK(selectable.IsSelected());
  DALI_TEST_CHECK(group.GetSelectedMember() == view);

  // Second tap on the same member is a no-op: it stays selected and the group
  // is never emptied by a gesture.
  TestGenerateTap(application, 50.0f, 50.0f, 300);
  DALI_TEST_CHECK(selectable.IsSelected());
  DALI_TEST_CHECK(group.GetSelectedMember() == view);
  END_TEST;
}

// Click on a grouped member always SELECTS (never toggles), so a click can never
// request deselection.
int UtcDaliGroupSelectableTraitClickSelectsP(void)
{
  UiTestApplication application;
  View              view = CreateSceneView(application, 0.0f, 0.0f);

  SelectionGroup group = SelectionGroup::New();
  group.Add(view);

  SelectableTrait selectable = view.AsSelectable();
  DALI_TEST_CHECK(!selectable.IsSelected());

  TestGenerateTap(application, 50.0f, 50.0f, 100);
  DALI_TEST_CHECK(selectable.IsSelected());
  END_TEST;
}

// ============================================================================
// Programmatic clear / empty (allowed only via explicit API)
// ============================================================================

// Programmatic SetSelected(false) on the winner empties the group.
int UtcDaliGroupSelectableTraitProgrammaticDeselectEmptiesGroupP(void)
{
  UiTestApplication application;
  View              view = CreateSceneView(application, 0.0f, 0.0f);

  SelectionGroup group = SelectionGroup::New();
  group.Add(view);

  SelectableTrait selectable = view.AsSelectable();
  selectable.SetSelected(true);
  DALI_TEST_CHECK(group.GetSelectedMember() == view);

  selectable.SetSelected(false);
  DALI_TEST_CHECK(!selectable.IsSelected());
  DALI_TEST_CHECK(!group.GetSelectedMember());
  END_TEST;
}

// ============================================================================
// Accessibility: RADIO_BUTTON role + CHECKED bit RMW lock-step
// ============================================================================

// Joining a group stamps the RADIO_BUTTON accessibility role on the member.
int UtcDaliGroupSelectableTraitAccessibilityRadioRoleOnJoinP(void)
{
  UiTestApplication application;
  View              view = View::New();

  DALI_TEST_CHECK(!IsRadioButton(view));

  SelectionGroup group = SelectionGroup::New();
  group.Add(view);

  DALI_TEST_CHECK(IsRadioButton(view));
  END_TEST;
}

// CHECKED is kept in lock-step with the selected state, and the write is a
// read-modify-write so that the ENABLED bit is preserved.
int UtcDaliGroupSelectableTraitAccessibilityCheckedLockStepP(void)
{
  UiTestApplication application;
  View              view = CreateSceneView(application, 0.0f, 0.0f);

  SelectionGroup group = SelectionGroup::New();
  group.Add(view);

  SelectableTrait selectable = view.AsSelectable();

  // Default: ENABLED set, CHECKED clear.
  DALI_TEST_CHECK(IsEnabledStateSet(view));
  DALI_TEST_CHECK(!IsChecked(view));

  selectable.SetSelected(true);
  DALI_TEST_CHECK(IsChecked(view));
  // RMW must preserve the ENABLED bit.
  DALI_TEST_CHECK(IsEnabledStateSet(view));

  selectable.SetSelected(false);
  DALI_TEST_CHECK(!IsChecked(view));
  DALI_TEST_CHECK(IsEnabledStateSet(view));
  END_TEST;
}

// A member that joins selected into a group that already has a winner is forced
// deselected, so it ends up RADIO_BUTTON but NOT CHECKED (M2 ordering).
int UtcDaliGroupSelectableTraitAccessibilityCheckedOnJoinWithWinnerP(void)
{
  UiTestApplication application;
  View              a = CreateSceneView(application, 0.0f, 0.0f);
  View              b = CreateSceneView(application, 200.0f, 0.0f);

  SelectionGroup group = SelectionGroup::New();
  group.Add(a);
  a.AsSelectable().SetSelected(true);

  // b is selected before joining a group that already has winner a.
  b.AsSelectable().SetSelected(true);
  group.Add(b);

  // b must be RADIO_BUTTON, NOT CHECKED, NOT selected; ENABLED preserved.
  DALI_TEST_CHECK(IsRadioButton(b));
  DALI_TEST_CHECK(!IsChecked(b));
  DALI_TEST_CHECK(!b.AsSelectable().IsSelected());
  DALI_TEST_CHECK(IsEnabledStateSet(b));

  // The seed winner a is RADIO_BUTTON and CHECKED.
  DALI_TEST_CHECK(IsRadioButton(a));
  DALI_TEST_CHECK(IsChecked(a));
  END_TEST;
}

// A member that joins selected into an empty group is the seed winner: it ends up
// RADIO_BUTTON and CHECKED.
int UtcDaliGroupSelectableTraitAccessibilityCheckedOnSeedJoinP(void)
{
  UiTestApplication application;
  View              a = CreateSceneView(application, 0.0f, 0.0f);

  a.AsSelectable().SetSelected(true);

  SelectionGroup group = SelectionGroup::New();
  group.Add(a);

  DALI_TEST_CHECK(IsRadioButton(a));
  DALI_TEST_CHECK(IsChecked(a));
  END_TEST;
}

// Leaving the group restores the role (to NONE) and clears the CHECKED bit while
// preserving ENABLED.
int UtcDaliGroupSelectableTraitAccessibilityRestoreOnLeaveP(void)
{
  UiTestApplication application;
  View              view = CreateSceneView(application, 0.0f, 0.0f);

  SelectionGroup group = SelectionGroup::New();
  group.Add(view);

  SelectableTrait selectable = view.AsSelectable();
  selectable.SetSelected(true);
  DALI_TEST_CHECK(IsRadioButton(view));
  DALI_TEST_CHECK(IsChecked(view));

  group.Remove(view);

  DALI_TEST_CHECK(!IsRadioButton(view));
  DALI_TEST_CHECK(!IsChecked(view));
  DALI_TEST_CHECK(IsEnabledStateSet(view));
  END_TEST;
}

// ============================================================================
// Membership-gated interaction: ungrouped behaves as plain Selectable,
// Add() switches to select-only, Remove() restores pre-join behaviour.
// ============================================================================

// An ungrouped GroupSelectable View (AsGroupSelectable() but never Add()ed) behaves
// exactly as a plain Selectable: a click SELECTS, a second click DESELECTS (toggle).
// This is the corrected contract: interaction wiring is gated on group MEMBERSHIP, not
// on the mere presence of GroupSelectableTrait.
int UtcDaliGroupSelectableTraitUngroupedTogglesByClickP(void)
{
  UiTestApplication application;
  View              view = CreateSceneView(application, 0.0f, 0.0f);

  // Upgrade to GroupSelectable but do NOT add it to any group.
  GroupSelectableTrait trait      = view.AsGroupSelectable();
  SelectableTrait      selectable = view.AsSelectable();
  DALI_TEST_CHECK(!trait.GetGroup());
  DALI_TEST_CHECK(!selectable.IsSelected());

  // First tap selects (plain Selectable toggle, since toggle-by-click is left intact).
  TestGenerateTap(application, 50.0f, 50.0f, 100);
  DALI_TEST_CHECK(selectable.IsSelected());

  // Second tap deselects (toggle) - an ungrouped member is NOT select-only.
  TestGenerateTap(application, 50.0f, 50.0f, 300);
  DALI_TEST_CHECK(!selectable.IsSelected());
  END_TEST;
}

// Add() switches the member to select-only: a click selects, and re-click is a no-op
// (still selected; the group is never emptied by a gesture).
int UtcDaliGroupSelectableTraitGroupedIsSelectOnlyP(void)
{
  UiTestApplication application;
  View              view = CreateSceneView(application, 0.0f, 0.0f);

  SelectionGroup group = SelectionGroup::New();
  group.Add(view);

  SelectableTrait selectable = view.AsSelectable();
  DALI_TEST_CHECK(!selectable.IsSelected());

  // First tap selects.
  TestGenerateTap(application, 50.0f, 50.0f, 100);
  DALI_TEST_CHECK(selectable.IsSelected());
  DALI_TEST_CHECK(group.GetSelectedMember() == view);

  // Re-click is a no-op: still selected, group not emptied.
  TestGenerateTap(application, 50.0f, 50.0f, 300);
  DALI_TEST_CHECK(selectable.IsSelected());
  DALI_TEST_CHECK(group.GetSelectedMember() == view);
  END_TEST;
}

// Remove() restores toggle-by-click behaviour: after removal a click toggles again.
int UtcDaliGroupSelectableTraitRemoveRestoresToggleP(void)
{
  UiTestApplication application;
  View              view = CreateSceneView(application, 0.0f, 0.0f);

  SelectionGroup group = SelectionGroup::New();
  group.Add(view);

  SelectableTrait selectable = view.AsSelectable();
  DALI_TEST_CHECK(selectable.IsToggleByClickEnabled() == false); // forced off while grouped

  group.Remove(view);

  // Toggle-by-click restored to its pre-join default (true).
  DALI_TEST_CHECK(selectable.IsToggleByClickEnabled() == true);

  // A click now toggles again: select then deselect.
  TestGenerateTap(application, 50.0f, 50.0f, 100);
  DALI_TEST_CHECK(selectable.IsSelected());
  TestGenerateTap(application, 50.0f, 50.0f, 300);
  DALI_TEST_CHECK(!selectable.IsSelected());
  END_TEST;
}

// Remove() restores the EXACT pre-join toggle-by-click value, not unconditionally true:
// if toggle-by-click was disabled before Add(), it stays disabled after Remove().
int UtcDaliGroupSelectableTraitRemoveRestoresToggleFalseP(void)
{
  UiTestApplication application;
  View              view = CreateSceneView(application, 0.0f, 0.0f);

  // Disable toggle-by-click BEFORE joining the group.
  SelectableTrait selectable = view.AsSelectable();
  selectable.EnableToggleByClick(false);
  DALI_TEST_CHECK(selectable.IsToggleByClickEnabled() == false);

  SelectionGroup group = SelectionGroup::New();
  group.Add(view);

  group.Remove(view);

  // The saved (false) value is restored, NOT forced true.
  DALI_TEST_CHECK(selectable.IsToggleByClickEnabled() == false);
  END_TEST;
}

// AsInteractive().ClickedSignal() and AsSelectable().SelectionChangedSignal() user
// callbacks must still fire after AsGroupSelectable() + Add(): the trait only ever
// disconnects its OWN handlers, never user-connected ones.
int UtcDaliGroupSelectableTraitUserSignalsSurviveGroupingP(void)
{
  UiTestApplication application;
  View              view = CreateSceneView(application, 0.0f, 0.0f);

  // Connect user callbacks BEFORE grouping.
  bool clickedCalled   = false;
  bool selectionCalled = false;

  InteractiveTrait interactive = view.AsInteractive();
  interactive.ClickedSignal().Connect(&application, [&clickedCalled](View, InputEvent) {
    clickedCalled = true;
  });

  SelectableTrait selectable = view.AsSelectable();
  selectable.SelectionChangedSignal().Connect(&application, [&selectionCalled](View, bool, InputEvent) {
    selectionCalled = true;
  });

  SelectionGroup group = SelectionGroup::New();
  group.Add(view);

  // A gesture click on the grouped member must still fire BOTH user callbacks.
  TestGenerateTap(application, 50.0f, 50.0f, 100);
  DALI_TEST_CHECK(clickedCalled);
  DALI_TEST_CHECK(selectionCalled);
  DALI_TEST_CHECK(selectable.IsSelected());
  END_TEST;
}

// FIX-5 postcondition: with a default-role View, after Remove(winner) the logical
// SELECTED state is preserved (IsSelected()==true), the role is restored to its pre-join
// value (NONE), and the a11y CHECKED bit follows that restored non-checkable role (false).
int UtcDaliGroupSelectableTraitRemoveWinnerPreservesSelectedClearsCheckedP(void)
{
  UiTestApplication application;
  View              view = CreateSceneView(application, 0.0f, 0.0f);

  SelectionGroup group = SelectionGroup::New();
  group.Add(view);

  SelectableTrait selectable = view.AsSelectable();
  selectable.SetSelected(true);
  DALI_TEST_CHECK(group.GetSelectedMember() == view);
  DALI_TEST_CHECK(IsRadioButton(view));
  DALI_TEST_CHECK(IsChecked(view));

  group.Remove(view);

  // Logical SELECTED state is preserved.
  DALI_TEST_CHECK(selectable.IsSelected());
  // Role restored to pre-join NONE (default).
  DALI_TEST_CHECK(view.GetProperty<int>(View::Property::ACCESSIBILITY_ROLE) ==
                  static_cast<int>(AccessibilityRole::NONE));
  // CHECKED follows the restored non-checkable role -> cleared, even though selected.
  DALI_TEST_CHECK(!IsChecked(view));
  // ENABLED preserved by the RMW.
  DALI_TEST_CHECK(IsEnabledStateSet(view));
  END_TEST;
}

// ============================================================================
// P2-2: toggle-by-click is LOCKED while grouped, restored on leave
// ============================================================================

// P2-2a: a grouped member's EnableToggleByClick(true) cannot empty the group by gesture.
// While grouped, EnableToggleByClick(true) is ignored (locked), so OnClickedForToggle is
// never reconnected. A tap on the selected winner stays selected (select-only) and the
// group is never emptied by a gesture.
//
// Both members occupy the scene origin (a bare DALi UI View added to the Scene is not
// repositioned by Actor::Property::POSITION, mirroring UtcDaliSelectionGroupSingleSelection-
// ByTapP), so a tap is received by the TOPMOST member (the one added last). The winner is
// therefore added LAST so the tap lands on it.
int UtcDaliGroupSelectableTraitGroupedToggleEnableIgnoredP(void)
{
  UiTestApplication application;
  View              other  = CreateSceneView(application, 0.0f, 0.0f);
  View              winner = CreateSceneView(application, 0.0f, 0.0f);

  SelectionGroup group = SelectionGroup::New();
  group.Add(other);
  group.Add(winner); // added last -> topmost -> receives the tap

  SelectableTrait selectable = winner.AsSelectable();

  // Select the winner.
  selectable.SetSelected(true);
  DALI_TEST_CHECK(group.GetSelectedMember() == winner);

  // Attempt to re-enable toggle-by-click while grouped: this is IGNORED (locked), so it
  // does not reconnect the toggle handler and stays reported as off.
  selectable.EnableToggleByClick(true);
  DALI_TEST_CHECK(selectable.IsToggleByClickEnabled() == false);

  // Tap the winner: select-only handler keeps it selected; the group is NOT emptied (the
  // pre-fix toggle handler would have deselected it and emptied the group here).
  TestGenerateTap(application, 50.0f, 50.0f, 100);
  DALI_TEST_CHECK(selectable.IsSelected());
  DALI_TEST_CHECK(group.GetSelectedMember() == winner);

  // A second tap is still a no-op (true radio): never empties the group by gesture.
  TestGenerateTap(application, 50.0f, 50.0f, 300);
  DALI_TEST_CHECK(selectable.IsSelected());
  DALI_TEST_CHECK(group.GetSelectedMember() == winner);
  END_TEST;
}

// P2-2b: leaving the group clears the lock and restores the pre-join toggle value, so the
// member behaves as a normal Selectable again. A member that was toggle-by-click ENABLED
// before Add() (the default) toggles by click again after Remove() AND can have toggle
// re-disabled (lock cleared); a member DISABLED before Add() has its (false) value
// restored, NOT forced true.
int UtcDaliGroupSelectableTraitLeaveUnlocksAndRestoresToggleP(void)
{
  UiTestApplication application;

  // -- Default member (toggle-by-click ENABLED before Add). Single scene view so the tap
  //    lands on it unambiguously. --
  View            defaultMember = CreateSceneView(application, 0.0f, 0.0f);
  SelectableTrait defSelectable = defaultMember.AsSelectable();
  DALI_TEST_CHECK(defSelectable.IsToggleByClickEnabled() == true); // default

  SelectionGroup group = SelectionGroup::New();
  group.Add(defaultMember);
  // While grouped toggle is forced off and locked.
  DALI_TEST_CHECK(defSelectable.IsToggleByClickEnabled() == false);
  // EnableToggleByClick is ignored while locked.
  defSelectable.EnableToggleByClick(true);
  DALI_TEST_CHECK(defSelectable.IsToggleByClickEnabled() == false);

  group.Remove(defaultMember);
  // After leaving, the lock is cleared and the pre-join ENABLED value is restored.
  DALI_TEST_CHECK(defSelectable.IsToggleByClickEnabled() == true);
  // A click toggles again: select then deselect.
  TestGenerateTap(application, 50.0f, 50.0f, 100);
  DALI_TEST_CHECK(defSelectable.IsSelected());
  TestGenerateTap(application, 50.0f, 50.0f, 300);
  DALI_TEST_CHECK(!defSelectable.IsSelected());
  // The lock is cleared, so EnableToggleByClick is honored again (disable it).
  defSelectable.EnableToggleByClick(false);
  DALI_TEST_CHECK(defSelectable.IsToggleByClickEnabled() == false);

  // -- Disabled member (toggle-by-click DISABLED before Add). --
  View            disabledMember = CreateSceneView(application, 0.0f, 0.0f);
  SelectableTrait disSelectable  = disabledMember.AsSelectable();
  disSelectable.EnableToggleByClick(false);
  DALI_TEST_CHECK(disSelectable.IsToggleByClickEnabled() == false);

  SelectionGroup group2 = SelectionGroup::New();
  group2.Add(disabledMember);
  DALI_TEST_CHECK(disSelectable.IsToggleByClickEnabled() == false);

  group2.Remove(disabledMember);
  // The saved (false) value is restored, NOT forced true; the lock is cleared so a
  // subsequent EnableToggleByClick is honored again.
  DALI_TEST_CHECK(disSelectable.IsToggleByClickEnabled() == false);
  disSelectable.EnableToggleByClick(true);
  DALI_TEST_CHECK(disSelectable.IsToggleByClickEnabled() == true);
  END_TEST;
}
