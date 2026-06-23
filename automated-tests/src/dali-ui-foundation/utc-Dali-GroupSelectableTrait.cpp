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

// Binds an on-scene View to the named group (the replacement for SelectionGroup::Add()).
View CreateNamedSceneMember(UiTestApplication& application, const std::string& name, float x = 0.0f, float y = 0.0f)
{
  View view = CreateSceneView(application, x, y);
  view.AsGroupSelectable().SetGroupName(name);
  return view;
}

// Adds a child View to an on-scene View parent and pumps the frame so the child's scene
// connection (and hence parent auto-grouping) takes effect.
void AddChild(UiTestApplication& application, View parent, View child)
{
  parent.Add(child);
  application.SendNotification();
  application.Render();
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
// SetGroupName / GetGroupName / GetGroup
// ============================================================================

int UtcDaliGroupSelectableTraitGroupNameEmptyByDefaultP(void)
{
  UiTestApplication application;
  View              view = View::New();

  GroupSelectableTrait trait = view.AsGroupSelectable();
  DALI_TEST_CHECK(trait.GetGroupName().empty());
  END_TEST;
}

int UtcDaliGroupSelectableTraitGetGroupEmptyByDefaultP(void)
{
  UiTestApplication application;
  View              view = View::New();

  GroupSelectableTrait trait = view.AsGroupSelectable();
  DALI_TEST_CHECK(!trait.GetGroup());
  END_TEST;
}

int UtcDaliGroupSelectableTraitSetGroupNameBindsP(void)
{
  UiTestApplication application;
  View              view = View::New();

  GroupSelectableTrait trait = view.AsGroupSelectable();
  trait.SetGroupName("UtcSetName");

  DALI_TEST_EQUALS(trait.GetGroupName(), std::string("UtcSetName"), TEST_LOCATION);

  SelectionGroup group = SelectionGroup::Find("UtcSetName");
  SelectionGroup got   = trait.GetGroup();
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
  View              view = CreateNamedSceneMember(application, "UtcReclick", 0.0f, 0.0f);

  GroupSelectableTrait trait      = view.AsGroupSelectable();
  SelectableTrait      selectable = view.AsSelectable();
  SelectionGroup       group      = SelectionGroup::Find("UtcReclick");

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
  View              view = CreateNamedSceneMember(application, "UtcClickSelects", 0.0f, 0.0f);

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
  View              view = CreateNamedSceneMember(application, "UtcProgDeselect", 0.0f, 0.0f);

  SelectionGroup  group      = SelectionGroup::Find("UtcProgDeselect");
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

  view.AsGroupSelectable().SetGroupName("UtcA11yRadio");

  DALI_TEST_CHECK(IsRadioButton(view));
  END_TEST;
}

// CHECKED is kept in lock-step with the selected state, and the write is a
// read-modify-write so that the ENABLED bit is preserved.
int UtcDaliGroupSelectableTraitAccessibilityCheckedLockStepP(void)
{
  UiTestApplication application;
  View              view = CreateNamedSceneMember(application, "UtcA11yLockStep", 0.0f, 0.0f);

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
  View              a = CreateNamedSceneMember(application, "UtcA11yJoinWinner", 0.0f, 0.0f);
  View              b = CreateSceneView(application, 200.0f, 0.0f);

  a.AsSelectable().SetSelected(true);

  // b is selected before joining a group that already has winner a.
  b.AsSelectable().SetSelected(true);
  b.AsGroupSelectable().SetGroupName("UtcA11yJoinWinner");

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

  a.AsGroupSelectable().SetGroupName("UtcA11ySeedJoin");

  DALI_TEST_CHECK(IsRadioButton(a));
  DALI_TEST_CHECK(IsChecked(a));
  END_TEST;
}

// Leaving the group restores the role (to NONE) and clears the CHECKED bit while
// preserving ENABLED.
int UtcDaliGroupSelectableTraitAccessibilityRestoreOnLeaveP(void)
{
  UiTestApplication application;
  View              view = CreateNamedSceneMember(application, "UtcA11yRestore", 0.0f, 0.0f);

  SelectableTrait selectable = view.AsSelectable();
  selectable.SetSelected(true);
  DALI_TEST_CHECK(IsRadioButton(view));
  DALI_TEST_CHECK(IsChecked(view));

  // Clear the name: the scene parent is the (non-View) scene root, so no parent-auto
  // fallback -> the member fully leaves the group.
  view.AsGroupSelectable().SetGroupName("");

  DALI_TEST_CHECK(!IsRadioButton(view));
  DALI_TEST_CHECK(!IsChecked(view));
  DALI_TEST_CHECK(IsEnabledStateSet(view));
  END_TEST;
}

// ============================================================================
// Membership-gated interaction: ungrouped behaves as plain Selectable,
// SetGroupName switches to select-only, clear-name restores pre-join behaviour.
// ============================================================================

// An ungrouped GroupSelectable View (AsGroupSelectable() but no name and no on-scene View
// parent) behaves exactly as a plain Selectable: a click SELECTS, a second click DESELECTS
// (toggle). Interaction wiring is gated on group MEMBERSHIP, not on the mere presence of
// GroupSelectableTrait.
int UtcDaliGroupSelectableTraitUngroupedTogglesByClickP(void)
{
  UiTestApplication application;
  View              view = CreateSceneView(application, 0.0f, 0.0f);

  // Upgrade to GroupSelectable but do NOT name a group; the scene parent is the non-View
  // scene root, so parent-auto does not apply -> the member is ungrouped.
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

// SetGroupName switches the member to select-only: a click selects, and re-click is a no-op
// (still selected; the group is never emptied by a gesture).
int UtcDaliGroupSelectableTraitGroupedIsSelectOnlyP(void)
{
  UiTestApplication application;
  View              view = CreateNamedSceneMember(application, "UtcSelectOnly", 0.0f, 0.0f);

  SelectionGroup  group      = SelectionGroup::Find("UtcSelectOnly");
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

// Clearing the name restores toggle-by-click behaviour: after leaving the group a click
// toggles again.
int UtcDaliGroupSelectableTraitRemoveRestoresToggleP(void)
{
  UiTestApplication application;
  View              view = CreateNamedSceneMember(application, "UtcRestoreToggle", 0.0f, 0.0f);

  SelectableTrait selectable = view.AsSelectable();
  DALI_TEST_CHECK(selectable.IsToggleByClickEnabled() == false); // forced off while grouped

  view.AsGroupSelectable().SetGroupName("");

  // Toggle-by-click restored to its pre-join default (true).
  DALI_TEST_CHECK(selectable.IsToggleByClickEnabled() == true);

  // A click now toggles again: select then deselect.
  TestGenerateTap(application, 50.0f, 50.0f, 100);
  DALI_TEST_CHECK(selectable.IsSelected());
  TestGenerateTap(application, 50.0f, 50.0f, 300);
  DALI_TEST_CHECK(!selectable.IsSelected());
  END_TEST;
}

// Clearing the name restores the EXACT pre-join toggle-by-click value, not unconditionally
// true: if toggle-by-click was disabled before joining, it stays disabled after leaving.
int UtcDaliGroupSelectableTraitRemoveRestoresToggleFalseP(void)
{
  UiTestApplication application;
  View              view = CreateSceneView(application, 0.0f, 0.0f);

  // Disable toggle-by-click BEFORE joining the group.
  SelectableTrait selectable = view.AsSelectable();
  selectable.EnableToggleByClick(false);
  DALI_TEST_CHECK(selectable.IsToggleByClickEnabled() == false);

  view.AsGroupSelectable().SetGroupName("UtcRestoreToggleFalse");

  view.AsGroupSelectable().SetGroupName("");

  // The saved (false) value is restored, NOT forced true.
  DALI_TEST_CHECK(selectable.IsToggleByClickEnabled() == false);
  END_TEST;
}

// AsInteractive().ClickedSignal() and AsSelectable().SelectionChangedSignal() user
// callbacks must still fire after AsGroupSelectable() + SetGroupName(): the trait only ever
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

  view.AsGroupSelectable().SetGroupName("UtcUserSignals");

  // A gesture click on the grouped member must still fire BOTH user callbacks.
  TestGenerateTap(application, 50.0f, 50.0f, 100);
  DALI_TEST_CHECK(clickedCalled);
  DALI_TEST_CHECK(selectionCalled);
  DALI_TEST_CHECK(selectable.IsSelected());
  END_TEST;
}

// FIX-5 postcondition: with a default-role View, after clearing the winner's name the
// logical SELECTED state is preserved (IsSelected()==true), the role is restored to its
// pre-join value (NONE), and the a11y CHECKED bit follows that restored non-checkable role
// (false).
int UtcDaliGroupSelectableTraitRemoveWinnerPreservesSelectedClearsCheckedP(void)
{
  UiTestApplication application;
  View              view = CreateNamedSceneMember(application, "UtcRemoveWinnerA11y", 0.0f, 0.0f);

  SelectionGroup  group      = SelectionGroup::Find("UtcRemoveWinnerA11y");
  SelectableTrait selectable = view.AsSelectable();
  selectable.SetSelected(true);
  DALI_TEST_CHECK(group.GetSelectedMember() == view);
  DALI_TEST_CHECK(IsRadioButton(view));
  DALI_TEST_CHECK(IsChecked(view));

  view.AsGroupSelectable().SetGroupName("");

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
// repositioned by Actor::Property::POSITION), so a tap is received by the TOPMOST member
// (the one added last). The winner is therefore added LAST so the tap lands on it.
int UtcDaliGroupSelectableTraitGroupedToggleEnableIgnoredP(void)
{
  UiTestApplication application;
  View              other  = CreateNamedSceneMember(application, "UtcToggleLock", 0.0f, 0.0f);
  View              winner = CreateNamedSceneMember(application, "UtcToggleLock", 0.0f, 0.0f); // added last -> topmost

  SelectionGroup  group      = SelectionGroup::Find("UtcToggleLock");
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
// before joining (the default) toggles by click again after leaving AND can have toggle
// re-disabled (lock cleared); a member DISABLED before joining has its (false) value
// restored, NOT forced true.
int UtcDaliGroupSelectableTraitLeaveUnlocksAndRestoresToggleP(void)
{
  UiTestApplication application;

  // -- Default member (toggle-by-click ENABLED before join). Single scene view so the tap
  //    lands on it unambiguously. --
  View            defaultMember = CreateNamedSceneMember(application, "UtcUnlockDefault", 0.0f, 0.0f);
  SelectableTrait defSelectable = defaultMember.AsSelectable();
  // While grouped toggle is forced off and locked.
  DALI_TEST_CHECK(defSelectable.IsToggleByClickEnabled() == false);
  // EnableToggleByClick is ignored while locked.
  defSelectable.EnableToggleByClick(true);
  DALI_TEST_CHECK(defSelectable.IsToggleByClickEnabled() == false);

  defaultMember.AsGroupSelectable().SetGroupName("");
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

  // -- Disabled member (toggle-by-click DISABLED before join). --
  View            disabledMember = CreateSceneView(application, 0.0f, 0.0f);
  SelectableTrait disSelectable  = disabledMember.AsSelectable();
  disSelectable.EnableToggleByClick(false);
  DALI_TEST_CHECK(disSelectable.IsToggleByClickEnabled() == false);

  disabledMember.AsGroupSelectable().SetGroupName("UtcUnlockDisabled");
  DALI_TEST_CHECK(disSelectable.IsToggleByClickEnabled() == false);

  disabledMember.AsGroupSelectable().SetGroupName("");
  // The saved (false) value is restored, NOT forced true; the lock is cleared so a
  // subsequent EnableToggleByClick is honored again.
  DALI_TEST_CHECK(disSelectable.IsToggleByClickEnabled() == false);
  disSelectable.EnableToggleByClick(true);
  DALI_TEST_CHECK(disSelectable.IsToggleByClickEnabled() == true);
  END_TEST;
}

// ============================================================================
// Lifecycle: parent auto-grouping (default), name precedence, scene scope
// ============================================================================

// Two children under one on-scene View parent (no name) auto-join the parent's group and
// are mutually exclusive.
int UtcDaliGroupSelectableTraitParentAutoGroupP(void)
{
  UiTestApplication application;
  View              parent = CreateSceneView(application, 0.0f, 0.0f);

  View a = CreateView();
  View b = CreateView();
  a.AsGroupSelectable();
  b.AsGroupSelectable();
  AddChild(application, parent, a);
  AddChild(application, parent, b);

  SelectionGroup group = SelectionGroup::Find(parent);
  DALI_TEST_EQUALS(group.GetMemberCount(), 2u, TEST_LOCATION);
  DALI_TEST_CHECK(a.AsGroupSelectable().GetGroup() == group);
  DALI_TEST_CHECK(b.AsGroupSelectable().GetGroup() == group);

  // Mutual exclusion through the auto-group.
  a.AsSelectable().SetSelected(true);
  DALI_TEST_CHECK(group.GetSelectedMember() == a);
  b.AsSelectable().SetSelected(true);
  DALI_TEST_CHECK(group.GetSelectedMember() == b);
  DALI_TEST_CHECK(!a.AsSelectable().IsSelected());
  END_TEST;
}

// An explicit name overrides parent auto-grouping: a named child does NOT join the parent
// group; it joins the named group instead.
int UtcDaliGroupSelectableTraitExplicitNameOverridesParentP(void)
{
  UiTestApplication application;
  View              parent = CreateSceneView(application, 0.0f, 0.0f);

  View named = CreateView();
  named.AsGroupSelectable().SetGroupName("UtcOverride"); // name set before parenting
  View autoChild = CreateView();
  autoChild.AsGroupSelectable();
  AddChild(application, parent, named);
  AddChild(application, parent, autoChild);

  SelectionGroup parentGroup = SelectionGroup::Find(parent);
  SelectionGroup namedGroup  = SelectionGroup::Find("UtcOverride");

  // The auto child is in the parent group; the named child is in the named group.
  DALI_TEST_CHECK(autoChild.AsGroupSelectable().GetGroup() == parentGroup);
  DALI_TEST_CHECK(named.AsGroupSelectable().GetGroup() == namedGroup);
  DALI_TEST_CHECK(namedGroup != parentGroup);
  DALI_TEST_EQUALS(parentGroup.GetMemberCount(), 1u, TEST_LOCATION);
  DALI_TEST_EQUALS(namedGroup.GetMemberCount(), 1u, TEST_LOCATION);
  END_TEST;
}

// The same group name across DIFFERENT parents binds members into one cross-parent group.
int UtcDaliGroupSelectableTraitSameGroupNameCrossParentP(void)
{
  UiTestApplication application;
  View              parent1 = CreateSceneView(application, 0.0f, 0.0f);
  View              parent2 = CreateSceneView(application, 0.0f, 200.0f);

  View a = CreateView();
  View b = CreateView();
  a.AsGroupSelectable().SetGroupName("UtcCrossName");
  b.AsGroupSelectable().SetGroupName("UtcCrossName");
  AddChild(application, parent1, a);
  AddChild(application, parent2, b);

  SelectionGroup group = SelectionGroup::Find("UtcCrossName");
  DALI_TEST_EQUALS(group.GetMemberCount(), 2u, TEST_LOCATION);

  // Cross-parent mutual exclusion.
  a.AsSelectable().SetSelected(true);
  DALI_TEST_CHECK(group.GetSelectedMember() == a);
  b.AsSelectable().SetSelected(true);
  DALI_TEST_CHECK(group.GetSelectedMember() == b);
  DALI_TEST_CHECK(!a.AsSelectable().IsSelected());
  END_TEST;
}

// Changing the group name moves the member from one named group to another.
int UtcDaliGroupSelectableTraitChangeGroupNameP(void)
{
  UiTestApplication application;
  View              view = View::New();

  view.AsGroupSelectable().SetGroupName("UtcChangeA");
  SelectionGroup gA = SelectionGroup::Find("UtcChangeA");
  DALI_TEST_EQUALS(gA.GetMemberCount(), 1u, TEST_LOCATION);

  view.AsGroupSelectable().SetGroupName("UtcChangeB");
  SelectionGroup gB = SelectionGroup::Find("UtcChangeB");
  DALI_TEST_EQUALS(gA.GetMemberCount(), 0u, TEST_LOCATION);
  DALI_TEST_EQUALS(gB.GetMemberCount(), 1u, TEST_LOCATION);
  DALI_TEST_CHECK(view.AsGroupSelectable().GetGroup() == gB);
  DALI_TEST_EQUALS(view.AsGroupSelectable().GetGroupName(), std::string("UtcChangeB"), TEST_LOCATION);
  END_TEST;
}

// Clearing the name while on-scene under a View parent rejoins the parent auto-group.
int UtcDaliGroupSelectableTraitClearNameRejoinsParentOnSceneP(void)
{
  UiTestApplication application;
  View              parent = CreateSceneView(application, 0.0f, 0.0f);

  View child = CreateView();
  child.AsGroupSelectable().SetGroupName("UtcRejoinNamed");
  AddChild(application, parent, child);

  SelectionGroup namedGroup  = SelectionGroup::Find("UtcRejoinNamed");
  SelectionGroup parentGroup = SelectionGroup::Find(parent);
  DALI_TEST_CHECK(child.AsGroupSelectable().GetGroup() == namedGroup);

  // Clear the name on-scene: parent auto-grouping takes over immediately.
  child.AsGroupSelectable().SetGroupName("");
  DALI_TEST_CHECK(child.AsGroupSelectable().GetGroupName().empty());
  DALI_TEST_CHECK(child.AsGroupSelectable().GetGroup() == parentGroup);
  DALI_TEST_EQUALS(namedGroup.GetMemberCount(), 0u, TEST_LOCATION);
  DALI_TEST_EQUALS(parentGroup.GetMemberCount(), 1u, TEST_LOCATION);
  END_TEST;
}

// Clearing the name while OFF-scene leaves the member ungrouped; it auto-joins its parent
// only on the next scene connection.
int UtcDaliGroupSelectableTraitClearNameRejoinsParentOffSceneP(void)
{
  UiTestApplication application;
  View              parent = CreateSceneView(application, 0.0f, 0.0f);

  View child = CreateView();
  child.AsGroupSelectable().SetGroupName("UtcRejoinOff");

  // Clear name while off-scene: no parent yet, so the member is ungrouped.
  child.AsGroupSelectable().SetGroupName("");
  DALI_TEST_CHECK(!child.AsGroupSelectable().GetGroup());

  // Connect to the scene under a View parent: parent auto-grouping kicks in.
  AddChild(application, parent, child);
  SelectionGroup parentGroup = SelectionGroup::Find(parent);
  DALI_TEST_CHECK(child.AsGroupSelectable().GetGroup() == parentGroup);
  DALI_TEST_EQUALS(parentGroup.GetMemberCount(), 1u, TEST_LOCATION);
  END_TEST;
}

// Reparenting a parent-auto member from parent1 to parent2 moves it between the two parent
// groups.
int UtcDaliGroupSelectableTraitReparentP(void)
{
  UiTestApplication application;
  View              parent1 = CreateSceneView(application, 0.0f, 0.0f);
  View              parent2 = CreateSceneView(application, 0.0f, 200.0f);

  View child = CreateView();
  child.AsGroupSelectable();
  AddChild(application, parent1, child);

  SelectionGroup g1 = SelectionGroup::Find(parent1);
  DALI_TEST_CHECK(child.AsGroupSelectable().GetGroup() == g1);
  DALI_TEST_EQUALS(g1.GetMemberCount(), 1u, TEST_LOCATION);

  // Reparent: Add() to parent2 unparents from parent1 first (off-scene then on-scene).
  AddChild(application, parent2, child);
  SelectionGroup g2 = SelectionGroup::Find(parent2);
  DALI_TEST_CHECK(child.AsGroupSelectable().GetGroup() == g2);
  DALI_TEST_EQUALS(g1.GetMemberCount(), 0u, TEST_LOCATION);
  DALI_TEST_EQUALS(g2.GetMemberCount(), 1u, TEST_LOCATION);
  END_TEST;
}

// Scene disconnection leaves the parent auto-group (scene-scoped membership).
int UtcDaliGroupSelectableTraitSceneDisconnectLeavesParentGroupP(void)
{
  UiTestApplication application;
  View              parent = CreateSceneView(application, 0.0f, 0.0f);

  View child = CreateView();
  child.AsGroupSelectable();
  AddChild(application, parent, child);

  SelectionGroup group = SelectionGroup::Find(parent);
  DALI_TEST_EQUALS(group.GetMemberCount(), 1u, TEST_LOCATION);

  // Unparent the child off the scene: it leaves the parent auto-group.
  child.Unparent();
  application.SendNotification();
  application.Render();

  DALI_TEST_EQUALS(group.GetMemberCount(), 0u, TEST_LOCATION);
  DALI_TEST_CHECK(!child.AsGroupSelectable().GetGroup());

  // Reconnecting under the same parent rejoins the auto-group.
  AddChild(application, parent, child);
  DALI_TEST_EQUALS(group.GetMemberCount(), 1u, TEST_LOCATION);
  DALI_TEST_CHECK(child.AsGroupSelectable().GetGroup() == group);
  END_TEST;
}

// A plain Selectable sibling (NOT GroupSelectable) under the same parent is not part of the
// parent auto-group; only AsGroupSelectable() children participate.
int UtcDaliGroupSelectableTraitCheckboxNotGroupedN(void)
{
  UiTestApplication application;
  View              parent = CreateSceneView(application, 0.0f, 0.0f);

  View grouped  = CreateView();
  View checkbox = CreateView();
  grouped.AsGroupSelectable();
  checkbox.AsSelectable(); // plain selectable, never group-selectable
  AddChild(application, parent, grouped);
  AddChild(application, parent, checkbox);

  SelectionGroup group = SelectionGroup::Find(parent);

  // Only the group-selectable child is a member.
  DALI_TEST_EQUALS(group.GetMemberCount(), 1u, TEST_LOCATION);
  DALI_TEST_CHECK(!checkbox.IsGroupSelectable());

  // Selecting the plain checkbox does not affect the auto-group.
  grouped.AsSelectable().SetSelected(true);
  DALI_TEST_CHECK(group.GetSelectedMember() == grouped);
  checkbox.AsSelectable().SetSelected(true);
  DALI_TEST_CHECK(group.GetSelectedMember() == grouped); // unaffected
  DALI_TEST_CHECK(checkbox.AsSelectable().IsSelected());
  END_TEST;
}

// Named membership is NOT scene-scoped: it persists across scene disconnection/reconnection
// (unlike parent-auto), distinguishing the two precedence sources.
int UtcDaliGroupSelectableTraitNamedMembershipPersistsAcrossSceneP(void)
{
  UiTestApplication application;
  View              parent = CreateSceneView(application, 0.0f, 0.0f);

  View child = CreateView();
  child.AsGroupSelectable().SetGroupName("UtcPersist");
  AddChild(application, parent, child);

  SelectionGroup group = SelectionGroup::Find("UtcPersist");
  DALI_TEST_CHECK(child.AsGroupSelectable().GetGroup() == group);

  // Off-scene: named membership persists (it is not scene-scoped).
  child.Unparent();
  application.SendNotification();
  application.Render();
  DALI_TEST_CHECK(child.AsGroupSelectable().GetGroup() == group);
  DALI_TEST_EQUALS(group.GetMemberCount(), 1u, TEST_LOCATION);
  END_TEST;
}
