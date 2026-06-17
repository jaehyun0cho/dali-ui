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
#include <string>
#include <vector>

#include <dali-ui-foundation/dali-ui-foundation.h>
#include <dali-ui-foundation/integration-api/reserved-trait-id.h>
#include <dali-ui-foundation/integration-api/view-integ.h>
#include <dali-ui-foundation/public-api/group-selectable-trait.h>
#include <dali-ui-foundation/public-api/selectable-group.h>
#include <dali-ui-foundation/public-api/view-accessibility-enums.h>
#include <dali-ui-foundation/public-api/view-impl.h>
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

View CreateView(UiTestApplication& application)
{
  View view = View::New();
  view.SetRequestedWidth(100.0f);
  view.SetRequestedHeight(100.0f);
  view.SetProperty(Actor::Property::PIVOT, Pivot::TOP_LEFT);
  view.SetProperty(Actor::Property::PARENT_ORIGIN, ParentOrigin::TOP_LEFT);
  application.GetScene().Add(view);
  application.SendNotification();
  application.Render();
  return view;
}

bool IsSelected(View view)
{
  return view.AsSelectable().IsSelected();
}

// Group signal recorder
struct GroupSignalData
{
  void Reset()
  {
    called   = false;
    previous = View();
    current  = View();
  }
  bool called = false;
  View previous;
  View current;
  int  callCount = 0;
};

struct GroupSignalFunctor
{
  GroupSignalFunctor(GroupSignalData& data)
  : data(data)
  {
  }
  void operator()(View previous, View current, InputEvent event)
  {
    data.called   = true;
    data.previous = previous;
    data.current  = current;
    data.callCount++;
  }
  GroupSignalData& data;
};

// Ordering recorder shared across item + group signals
struct OrderLog
{
  std::vector<std::string> entries;
};

} // namespace

void utc_dali_selectablegroup_startup(void)
{
  test_return_value = TET_UNDEF;
}

void utc_dali_selectablegroup_cleanup(void)
{
  test_return_value = TET_PASS;
}

// ============================================================================
// Construction / Handle
// ============================================================================

int UtcDaliSelectableGroupNewP(void)
{
  UiTestApplication application;
  SelectableGroup   group = SelectableGroup::New();
  DALI_TEST_CHECK(group);
  END_TEST;
}

int UtcDaliSelectableGroupDefaultConstructorN(void)
{
  UiTestApplication application;
  SelectableGroup   group;
  DALI_TEST_CHECK(!group);
  END_TEST;
}

int UtcDaliSelectableGroupCopyConstructorP(void)
{
  UiTestApplication application;
  SelectableGroup   group = SelectableGroup::New();
  SelectableGroup   copy(group);
  DALI_TEST_CHECK(copy);
  DALI_TEST_CHECK(copy == group);
  END_TEST;
}

int UtcDaliSelectableGroupMoveConstructorP(void)
{
  UiTestApplication application;
  SelectableGroup   group = SelectableGroup::New();
  SelectableGroup   moved(std::move(group));
  DALI_TEST_CHECK(moved);
  END_TEST;
}

int UtcDaliSelectableGroupAssignmentP(void)
{
  UiTestApplication application;
  SelectableGroup   group = SelectableGroup::New();
  SelectableGroup   other;
  other = group;
  DALI_TEST_CHECK(other == group);
  END_TEST;
}

int UtcDaliSelectableGroupDownCastP(void)
{
  UiTestApplication application;
  SelectableGroup   group = SelectableGroup::New();
  BaseHandle        handle(group);
  SelectableGroup   downcast = SelectableGroup::DownCast(handle);
  DALI_TEST_CHECK(downcast);
  END_TEST;
}

int UtcDaliSelectableGroupDownCastN(void)
{
  UiTestApplication application;
  BaseHandle        handle;
  SelectableGroup   downcast = SelectableGroup::DownCast(handle);
  DALI_TEST_CHECK(!downcast);
  END_TEST;
}

// ============================================================================
// Membership (Add / Remove / GetMemberCount)
// ============================================================================

int UtcDaliSelectableGroupAddCreatesTraitP(void)
{
  UiTestApplication application;
  SelectableGroup   group = SelectableGroup::New();
  View              view  = CreateView(application);

  DALI_TEST_CHECK(!view.IsSelectable());
  DALI_TEST_CHECK(group.Add(view));
  DALI_TEST_CHECK(view.IsSelectable());

  GroupSelectableTrait gst = GroupSelectableTrait::DownCast(view.AsSelectable());
  DALI_TEST_CHECK(gst);
  DALI_TEST_EQUALS(group.GetMemberCount(), 1u, TEST_LOCATION);
  END_TEST;
}

int UtcDaliSelectableGroupAddEmptyViewN(void)
{
  UiTestApplication application;
  SelectableGroup   group = SelectableGroup::New();
  View              empty;
  DALI_TEST_CHECK(!group.Add(empty));
  DALI_TEST_EQUALS(group.GetMemberCount(), 0u, TEST_LOCATION);
  END_TEST;
}

int UtcDaliSelectableGroupAddPlainSelectableN(void)
{
  UiTestApplication application;
  SelectableGroup   group = SelectableGroup::New();
  View              view  = CreateView(application);

  // Plain selectable trait already present -> Add must fail and not replace it.
  view.AsSelectable();
  DALI_TEST_CHECK(view.IsSelectable());
  DALI_TEST_CHECK(!group.Add(view));
  DALI_TEST_EQUALS(group.GetMemberCount(), 0u, TEST_LOCATION);

  // Still a plain selectable, not a group trait.
  GroupSelectableTrait gst = GroupSelectableTrait::DownCast(view.AsSelectable());
  DALI_TEST_CHECK(!gst);
  END_TEST;
}

int UtcDaliSelectableGroupAddCrossGroupRebindP(void)
{
  UiTestApplication application;
  SelectableGroup   groupA = SelectableGroup::New();
  SelectableGroup   groupB = SelectableGroup::New();
  View              view   = CreateView(application);

  DALI_TEST_CHECK(groupA.Add(view));
  DALI_TEST_EQUALS(groupA.GetMemberCount(), 1u, TEST_LOCATION);

  // Re-add to group B: rebinds, leaving group A.
  DALI_TEST_CHECK(groupB.Add(view));
  DALI_TEST_EQUALS(groupB.GetMemberCount(), 1u, TEST_LOCATION);
  DALI_TEST_EQUALS(groupA.GetMemberCount(), 0u, TEST_LOCATION);

  GroupSelectableTrait gst = GroupSelectableTrait::DownCast(view.AsSelectable());
  DALI_TEST_CHECK(gst.GetGroup() == groupB);
  END_TEST;
}

int UtcDaliSelectableGroupRemoveResetsRadioAccessibilityP(void)
{
  UiTestApplication application;
  SelectableGroup   group = SelectableGroup::New();
  View              view  = CreateView(application);

  group.Add(view);
  group.SelectMember(view);
  application.SendNotification();
  application.Render();
  DALI_TEST_CHECK(IsSelected(view));
  DALI_TEST_CHECK(IsChecked(view));
  DALI_TEST_EQUALS(view.GetProperty<int>(View::Property::ACCESSIBILITY_ROLE),
                   static_cast<int>(AccessibilityRole::RADIO_BUTTON),
                   TEST_LOCATION);

  DALI_TEST_CHECK(group.Remove(view));
  DALI_TEST_EQUALS(group.GetMemberCount(), 0u, TEST_LOCATION);

  // Selected bool is preserved (becomes a standalone selectable)...
  DALI_TEST_CHECK(IsSelected(view));
  // ...but the radio accessibility is undone: CHECKED cleared, role restored to NONE.
  // CHECKED is cleared while the role is still RADIO_BUTTON, then the role is restored;
  // the AT-SPI state-changed emission ordering is not observable in this test harness,
  // so only the final observable bit value and role are asserted here.
  DALI_TEST_CHECK(!IsChecked(view));
  DALI_TEST_EQUALS(view.GetProperty<int>(View::Property::ACCESSIBILITY_ROLE),
                   static_cast<int>(AccessibilityRole::NONE),
                   TEST_LOCATION);

  // Group no longer reports it as selected.
  DALI_TEST_CHECK(!group.GetSelectedMember());

  // The orphaned selectable can now be toggled off freely (no longer a radio no-op).
  view.AsSelectable().SetSelected(false);
  DALI_TEST_CHECK(!IsSelected(view));
  END_TEST;
}

int UtcDaliSelectableGroupRemoveNonMemberN(void)
{
  UiTestApplication application;
  SelectableGroup   group = SelectableGroup::New();
  View              view  = CreateView(application);
  DALI_TEST_CHECK(!group.Remove(view));
  END_TEST;
}

int UtcDaliSelectableGroupAddSelectedMemberDoesNotStealP(void)
{
  UiTestApplication application;
  SelectableGroup   groupA = SelectableGroup::New();
  SelectableGroup   groupB = SelectableGroup::New();
  View              a      = CreateView(application);
  View              b      = CreateView(application);

  groupA.Add(a);
  groupA.SelectMember(a); // a is selected in group A

  groupB.Add(b);
  groupB.SelectMember(b); // b is group B's winner

  // Move the already-selected a into group B, which already has a winner (b):
  // the existing winner is kept and the newcomer is deselected.
  DALI_TEST_CHECK(groupB.Add(a));
  DALI_TEST_CHECK(groupB.GetSelectedMember() == b);
  DALI_TEST_CHECK(b.AsSelectable().IsSelected());
  DALI_TEST_CHECK(!a.AsSelectable().IsSelected());
  DALI_TEST_CHECK(!IsChecked(a));
  DALI_TEST_CHECK(IsChecked(b));
  END_TEST;
}

int UtcDaliSelectableGroupAddSelectedMemberBecomesWinnerWhenEmptyP(void)
{
  UiTestApplication application;
  SelectableGroup   groupA = SelectableGroup::New();
  SelectableGroup   groupB = SelectableGroup::New();
  View              a      = CreateView(application);

  groupA.Add(a);
  groupA.SelectMember(a); // a is selected in group A

  // Move the already-selected a into an empty group B: a becomes B's winner.
  DALI_TEST_CHECK(groupB.Add(a));
  DALI_TEST_CHECK(groupB.GetSelectedMember() == a);
  DALI_TEST_CHECK(a.AsSelectable().IsSelected());
  DALI_TEST_CHECK(IsChecked(a));
  END_TEST;
}

// ============================================================================
// Mutual exclusion
// ============================================================================

int UtcDaliSelectableGroupMutualExclusionP(void)
{
  UiTestApplication application;
  SelectableGroup   group = SelectableGroup::New();
  View              a     = CreateView(application);
  View              b     = CreateView(application);
  View              c     = CreateView(application);
  group.Add(a);
  group.Add(b);
  group.Add(c);
  DALI_TEST_EQUALS(group.GetMemberCount(), 3u, TEST_LOCATION);

  DALI_TEST_CHECK(group.SelectMember(b));
  DALI_TEST_CHECK(!IsSelected(a));
  DALI_TEST_CHECK(IsSelected(b));
  DALI_TEST_CHECK(!IsSelected(c));
  DALI_TEST_CHECK(group.GetSelectedMember() == b);

  // Selecting c deselects b.
  DALI_TEST_CHECK(group.SelectMember(c));
  DALI_TEST_CHECK(!IsSelected(a));
  DALI_TEST_CHECK(!IsSelected(b));
  DALI_TEST_CHECK(IsSelected(c));
  DALI_TEST_CHECK(group.GetSelectedMember() == c);
  END_TEST;
}

int UtcDaliSelectableGroupSelectNonMemberN(void)
{
  UiTestApplication application;
  SelectableGroup   group = SelectableGroup::New();
  View              a     = CreateView(application);
  View              b     = CreateView(application);
  group.Add(a);
  DALI_TEST_CHECK(!group.SelectMember(b)); // b is not a member
  END_TEST;
}

int UtcDaliSelectableGroupReselectIdempotentP(void)
{
  UiTestApplication application;
  SelectableGroup   group = SelectableGroup::New();
  View              a     = CreateView(application);
  group.Add(a);

  group.SelectMember(a);
  GroupSignalData    data;
  GroupSignalFunctor functor(data);
  group.SelectedMemberChangedSignal().Connect(&application, functor);

  // Re-selecting the current winner is a no-op (no signal).
  group.SelectMember(a);
  DALI_TEST_CHECK(!data.called);
  DALI_TEST_CHECK(IsSelected(a));
  END_TEST;
}

int UtcDaliSelectableGroupDifferentGroupsIndependentP(void)
{
  UiTestApplication application;
  SelectableGroup   g1 = SelectableGroup::New();
  SelectableGroup   g2 = SelectableGroup::New();
  View              a  = CreateView(application);
  View              b  = CreateView(application);
  g1.Add(a);
  g2.Add(b);

  g1.SelectMember(a);
  g2.SelectMember(b);

  // Each group keeps its own selection.
  DALI_TEST_CHECK(IsSelected(a));
  DALI_TEST_CHECK(IsSelected(b));
  DALI_TEST_CHECK(g1.GetSelectedMember() == a);
  DALI_TEST_CHECK(g2.GetSelectedMember() == b);
  END_TEST;
}

// ============================================================================
// Re-click / clear (winner deselect is a no-op)
// ============================================================================

int UtcDaliSelectableGroupWinnerDeselectIsNoOpP(void)
{
  UiTestApplication application;
  SelectableGroup   group = SelectableGroup::New();
  View              a     = CreateView(application);
  group.Add(a);
  group.SelectMember(a);

  GroupSignalData    data;
  GroupSignalFunctor functor(data);
  group.SelectedMemberChangedSignal().Connect(&application, functor);

  // Direct SetSelected(false) on the winner: vetoed, no change, no signal.
  a.AsSelectable().SetSelected(false);
  DALI_TEST_CHECK(IsSelected(a));
  DALI_TEST_CHECK(!data.called);
  DALI_TEST_CHECK(group.GetSelectedMember() == a);
  END_TEST;
}

int UtcDaliSelectableGroupClearSelectionP(void)
{
  UiTestApplication application;
  SelectableGroup   group = SelectableGroup::New();
  View              a     = CreateView(application);
  group.Add(a);
  group.SelectMember(a);
  DALI_TEST_CHECK(IsChecked(a));

  GroupSignalData    data;
  GroupSignalFunctor functor(data);
  group.SelectedMemberChangedSignal().Connect(&application, functor);

  DALI_TEST_CHECK(group.ClearSelection());
  DALI_TEST_CHECK(!IsSelected(a));
  DALI_TEST_CHECK(!IsChecked(a));
  DALI_TEST_CHECK(!group.GetSelectedMember());

  // Group signal: previous = a, current = empty.
  DALI_TEST_CHECK(data.called);
  DALI_TEST_EQUALS(data.callCount, 1, TEST_LOCATION);
  DALI_TEST_CHECK(data.previous == a);
  DALI_TEST_CHECK(!data.current);
  END_TEST;
}

int UtcDaliSelectableGroupClearSelectionEmptyP(void)
{
  UiTestApplication application;
  SelectableGroup   group = SelectableGroup::New();
  // Clearing an already-empty group succeeds and does nothing.
  DALI_TEST_CHECK(group.ClearSelection());
  END_TEST;
}

int UtcDaliSelectableGroupClearSelectionNotAllowedN(void)
{
  UiTestApplication application;
  SelectableGroup   group = SelectableGroup::New();
  View              a     = CreateView(application);
  group.Add(a);
  group.SelectMember(a);

  group.SetAllowEmptySelection(false);
  DALI_TEST_CHECK(!group.IsEmptySelectionAllowed());

  // Clear is rejected when empty selection is not allowed.
  DALI_TEST_CHECK(!group.ClearSelection());
  DALI_TEST_CHECK(IsSelected(a));
  DALI_TEST_CHECK(group.GetSelectedMember() == a);
  END_TEST;
}

int UtcDaliSelectableGroupAllowEmptySelectionDefaultP(void)
{
  UiTestApplication application;
  SelectableGroup   group = SelectableGroup::New();
  DALI_TEST_CHECK(group.IsEmptySelectionAllowed()); // default true
  END_TEST;
}

// ============================================================================
// Signal ordering
// ============================================================================

int UtcDaliSelectableGroupSwapSignalOrderingP(void)
{
  UiTestApplication application;
  SelectableGroup   group = SelectableGroup::New();
  View              a     = CreateView(application);
  View              b     = CreateView(application);
  group.Add(a);
  group.Add(b);
  group.SelectMember(a);

  OrderLog log;

  a.AsSelectable().SelectionChangedSignal().Connect(
    &application, [&log](View v, bool selected, InputEvent e)
  {
    log.entries.push_back(selected ? "A:true" : "A:false");
  });
  b.AsSelectable().SelectionChangedSignal().Connect(
    &application, [&log](View v, bool selected, InputEvent e)
  {
    log.entries.push_back(selected ? "B:true" : "B:false");
  });
  group.SelectedMemberChangedSignal().Connect(
    &application, [&log](View prev, View cur, InputEvent e)
  {
    log.entries.push_back("group");
  });

  // Swap A -> B.
  group.SelectMember(b);

  // Expected order: loser A false -> winner B true -> group changed.
  DALI_TEST_EQUALS((int)log.entries.size(), 3, TEST_LOCATION);
  DALI_TEST_EQUALS(log.entries[0], std::string("A:false"), TEST_LOCATION);
  DALI_TEST_EQUALS(log.entries[1], std::string("B:true"), TEST_LOCATION);
  DALI_TEST_EQUALS(log.entries[2], std::string("group"), TEST_LOCATION);
  END_TEST;
}

int UtcDaliSelectableGroupFirstSelectionSignalP(void)
{
  UiTestApplication application;
  SelectableGroup   group = SelectableGroup::New();
  View              a     = CreateView(application);
  group.Add(a);

  GroupSignalData    data;
  GroupSignalFunctor functor(data);
  group.SelectedMemberChangedSignal().Connect(&application, functor);

  group.SelectMember(a);
  DALI_TEST_CHECK(data.called);
  DALI_TEST_CHECK(!data.previous); // no previous winner
  DALI_TEST_CHECK(data.current == a);
  END_TEST;
}

// ============================================================================
// Accessibility
// ============================================================================

int UtcDaliSelectableGroupAccessibilityRoleP(void)
{
  UiTestApplication application;
  SelectableGroup   group = SelectableGroup::New();
  View              a     = CreateView(application);
  group.Add(a);

  int role = a.GetProperty<int>(View::Property::ACCESSIBILITY_ROLE);
  DALI_TEST_EQUALS(role, static_cast<int>(AccessibilityRole::RADIO_BUTTON), TEST_LOCATION);
  END_TEST;
}

int UtcDaliSelectableGroupAccessibilityCheckedSwapP(void)
{
  UiTestApplication application;
  SelectableGroup   group = SelectableGroup::New();
  View              a     = CreateView(application);
  View              b     = CreateView(application);
  group.Add(a);
  group.Add(b);

  group.SelectMember(a);
  DALI_TEST_CHECK(IsChecked(a));
  DALI_TEST_CHECK(!IsChecked(b));

  // Swap: both the loser and the winner update CHECKED.
  group.SelectMember(b);
  DALI_TEST_CHECK(!IsChecked(a));
  DALI_TEST_CHECK(IsChecked(b));
  END_TEST;
}

int UtcDaliSelectableGroupAccessibilityPreservesEnabledP(void)
{
  UiTestApplication application;
  SelectableGroup   group = SelectableGroup::New();
  View              a     = CreateView(application);
  group.Add(a);
  group.SelectMember(a);

  // CHECKED set must not clear ENABLED.
  const int enabledMask = 1 << static_cast<int>(AccessibilityState::ENABLED);
  int       states      = a.GetProperty<int>(View::Property::ACCESSIBILITY_STATES);
  DALI_TEST_CHECK((states & enabledMask) != 0);
  DALI_TEST_CHECK((states & CHECKED_MASK) != 0);
  END_TEST;
}

// ============================================================================
// Lifecycle
// ============================================================================

int UtcDaliSelectableGroupSelectedMemberDetachedP(void)
{
  UiTestApplication application;
  SelectableGroup   group = SelectableGroup::New();
  View              a     = CreateView(application);
  group.Add(a);
  group.SelectMember(a);
  DALI_TEST_CHECK(group.GetSelectedMember() == a);
  DALI_TEST_EQUALS(group.GetMemberCount(), 1u, TEST_LOCATION);

  // Detaching the trait fires OnDetaching, which unregisters the member. (This is
  // the same UnregisterMember cleanup path taken when the owner View is destroyed.)
  IntegrationView::RemoveTrait(GetImpl(a), Dali::Ui::Integration::ReservedTraitId::SELECTABLE_TRAIT);

  DALI_TEST_CHECK(!group.GetSelectedMember());
  DALI_TEST_EQUALS(group.GetMemberCount(), 0u, TEST_LOCATION);
  END_TEST;
}

int UtcDaliSelectableGroupGroupOutlivesHandleP(void)
{
  UiTestApplication application;
  View              a = CreateView(application);
  View              b = CreateView(application);

  {
    SelectableGroup group = SelectableGroup::New();
    group.Add(a);
    group.Add(b);
    group.SelectMember(a);
    // Drop the application's group handle; members hold the group alive.
  }

  // Mutual exclusion still works through the members' strong reference to the group.
  b.AsSelectable().SetSelected(true);
  DALI_TEST_CHECK(!IsSelected(a));
  DALI_TEST_CHECK(IsSelected(b));
  END_TEST;
}

// ============================================================================
// Event propagation to the group signal
// ============================================================================

int UtcDaliSelectableGroupSignalEventProgrammaticIsNoneP(void)
{
  UiTestApplication application;
  SelectableGroup   group = SelectableGroup::New();
  View              a     = CreateView(application);
  group.Add(a);

  InputEventType groupEventType = InputEventType::TOUCH_EVENT; // sentinel != NONE
  group.SelectedMemberChangedSignal().Connect(
    &application, [&groupEventType](View prev, View cur, InputEvent e)
  {
    groupEventType = e.GetInputEventType();
  });

  // Programmatic selection: the group signal carries InputEvent::None().
  group.SelectMember(a);
  DALI_TEST_EQUALS(static_cast<int>(groupEventType), static_cast<int>(InputEventType::NONE), TEST_LOCATION);
  END_TEST;
}

int UtcDaliSelectableGroupSignalEventFromClickP(void)
{
  UiTestApplication application;
  SelectableGroup   group = SelectableGroup::New();
  View              a     = CreateView(application);
  group.Add(a);
  application.SendNotification();
  application.Render();

  bool           groupCalled    = false;
  InputEventType groupEventType = InputEventType::NONE;
  group.SelectedMemberChangedSignal().Connect(
    &application, [&groupCalled, &groupEventType](View prev, View cur, InputEvent e)
  {
    groupCalled    = true;
    groupEventType = e.GetInputEventType();
  });

  // Click-driven selection: the originating input event propagates to the group signal.
  TestGenerateTap(application, 50.0f, 50.0f, 100);
  DALI_TEST_CHECK(groupCalled);
  DALI_TEST_CHECK(groupEventType != InputEventType::NONE);
  END_TEST;
}

// ============================================================================
// Re-entrancy: initiator torn down inside the loser's signal must not strand
// ============================================================================

int UtcDaliSelectableGroupInitiatorDetachedDuringLoserSignalP(void)
{
  UiTestApplication application;
  SelectableGroup   group = SelectableGroup::New();
  View              a     = CreateView(application);
  View              b     = CreateView(application);
  View              c     = CreateView(application);
  group.Add(a);
  group.Add(b);
  group.Add(c);
  group.SelectMember(a);

  // When A is deselected, detach B (the in-flight initiator) from inside the handler.
  a.AsSelectable().SelectionChangedSignal().Connect(
    &application, [b](View v, bool selected, InputEvent e)
  {
    if(!selected)
    {
      View target = b;
      IntegrationView::RemoveTrait(GetImpl(target), Dali::Ui::Integration::ReservedTraitId::SELECTABLE_TRAIT);
    }
  });

  // Selecting B: A's deselect handler detaches B mid-transition. The transition must
  // be cancelled cleanly, NOT left permanently stranded. The selection is aborted, so
  // SelectMember reports false and the group ends with no winner.
  DALI_TEST_CHECK(!group.SelectMember(b));
  DALI_TEST_CHECK(!group.GetSelectedMember());
  DALI_TEST_CHECK(!a.AsSelectable().IsSelected());
  DALI_TEST_CHECK(!b.AsSelectable().IsSelected());

  // A subsequent selection must still work (the group is Idle, not frozen).
  DALI_TEST_CHECK(group.SelectMember(c));
  DALI_TEST_CHECK(c.AsSelectable().IsSelected());
  DALI_TEST_CHECK(group.GetSelectedMember() == c);
  DALI_TEST_CHECK(!a.AsSelectable().IsSelected());
  END_TEST;
}

int UtcDaliSelectableGroupRejectMutationDuringTransitionP(void)
{
  UiTestApplication application;
  SelectableGroup   group = SelectableGroup::New();
  View              a     = CreateView(application);
  View              b     = CreateView(application);
  View              c     = CreateView(application);
  View              d     = CreateView(application);
  group.Add(a);
  group.Add(b);
  group.Add(c);
  group.SelectMember(a);

  // Capture the result of every public mutation attempted mid-transition.
  bool selectResult = true, addResult = true, removeResult = true, clearResult = true;
  a.AsSelectable().SelectionChangedSignal().Connect(
    &application, [&](View v, bool selected, InputEvent e)
  {
    if(!selected) // fired while the a -> b swap is in progress (Transitioning)
    {
      selectResult = group.SelectMember(c);
      addResult    = group.Add(d);
      removeResult = group.Remove(b);
      clearResult  = group.ClearSelection();
    }
  });

  group.SelectMember(b);

  // Every mutation attempted during the transition must have been rejected...
  DALI_TEST_CHECK(!selectResult);
  DALI_TEST_CHECK(!addResult);
  DALI_TEST_CHECK(!removeResult);
  DALI_TEST_CHECK(!clearResult);
  // ...and the original swap must still have completed cleanly.
  DALI_TEST_CHECK(group.GetSelectedMember() == b);
  DALI_TEST_CHECK(IsSelected(b));
  DALI_TEST_CHECK(!IsSelected(a));
  DALI_TEST_EQUALS(group.GetMemberCount(), 3u, TEST_LOCATION); // d was never added
  END_TEST;
}

int UtcDaliSelectableGroupCheckedInterleaveOrderP(void)
{
  UiTestApplication application;
  SelectableGroup   group = SelectableGroup::New();
  View              a     = CreateView(application);
  View              b     = CreateView(application);
  group.Add(a);
  group.Add(b);
  group.SelectMember(a);

  // Record (a CHECKED, b CHECKED) at each signal point during the swap.
  std::vector<std::string> log;
  auto                     snapshot = [&](const char* tag)
  {
    log.push_back(std::string(tag) + ":a=" + (IsChecked(a) ? "1" : "0") + ",b=" + (IsChecked(b) ? "1" : "0"));
  };
  a.AsSelectable().SelectionChangedSignal().Connect(&application, [&](View v, bool s, InputEvent e)
  { if(!s) snapshot("Aitem"); });
  b.AsSelectable().SelectionChangedSignal().Connect(&application, [&](View v, bool s, InputEvent e)
  { if(s) snapshot("Bitem"); });
  group.SelectedMemberChangedSignal().Connect(&application, [&](View p, View c, InputEvent e)
  { snapshot("group"); });

  group.SelectMember(b); // swap a -> b

  // CHECKED is written in OnSelectionChanged, just after each per-item signal, so:
  // - at A's item signal A is not yet cleared; - by B's item signal A is already
  //   cleared and B not yet set; - by the group signal A is cleared and B is set.
  DALI_TEST_EQUALS((int)log.size(), 3, TEST_LOCATION);
  DALI_TEST_EQUALS(log[0], std::string("Aitem:a=1,b=0"), TEST_LOCATION);
  DALI_TEST_EQUALS(log[1], std::string("Bitem:a=0,b=0"), TEST_LOCATION);
  DALI_TEST_EQUALS(log[2], std::string("group:a=0,b=1"), TEST_LOCATION);
  END_TEST;
}

int UtcDaliSelectableGroupGetSelectedMemberDuringSwapReturnsProspectiveP(void)
{
  UiTestApplication application;
  SelectableGroup   group = SelectableGroup::New();
  View              a     = CreateView(application);
  View              b     = CreateView(application);
  group.Add(a);
  group.Add(b);
  group.SelectMember(a);

  View duringSwap;
  a.AsSelectable().SelectionChangedSignal().Connect(
    &application, [&](View v, bool selected, InputEvent e)
  {
    if(!selected) // a's deselect, mid-swap
    {
      duringSwap = group.GetSelectedMember();
    }
  });

  group.SelectMember(b); // swap a -> b

  // Mid-swap the group reports the prospective winner (b), and after, the committed one.
  DALI_TEST_CHECK(duringSwap == b);
  DALI_TEST_CHECK(group.GetSelectedMember() == b);
  END_TEST;
}

int UtcDaliSelectableGroupRebindEmitsNoSignalP(void)
{
  UiTestApplication application;
  SelectableGroup   groupA = SelectableGroup::New();
  SelectableGroup   groupB = SelectableGroup::New();
  View              a      = CreateView(application);
  groupA.Add(a);
  groupA.SelectMember(a);

  // Connect AFTER the initial selection so we only observe the rebind.
  GroupSignalData    dataA, dataB;
  GroupSignalFunctor funcA(dataA), funcB(dataB);
  groupA.SelectedMemberChangedSignal().Connect(&application, funcA);
  groupB.SelectedMemberChangedSignal().Connect(&application, funcB);

  // Rebind a from groupA into the empty groupB: groupA silently loses its winner,
  // groupB silently gains one. Neither group emits SelectedMemberChangedSignal.
  groupB.Add(a);

  DALI_TEST_EQUALS(dataA.callCount, 0, TEST_LOCATION);
  DALI_TEST_EQUALS(dataB.callCount, 0, TEST_LOCATION);
  DALI_TEST_CHECK(!groupA.GetSelectedMember());
  DALI_TEST_CHECK(groupB.GetSelectedMember() == a);
  END_TEST;
}

int UtcDaliSelectableGroupRemoveWinnerEmitsNoSignalP(void)
{
  UiTestApplication application;
  SelectableGroup   group = SelectableGroup::New();
  View              a     = CreateView(application);
  group.Add(a);
  group.SelectMember(a);

  GroupSignalData    data;
  GroupSignalFunctor functor(data);
  group.SelectedMemberChangedSignal().Connect(&application, functor);

  // Removing the current winner empties the group with no signal.
  DALI_TEST_CHECK(group.Remove(a));
  DALI_TEST_EQUALS(data.callCount, 0, TEST_LOCATION);
  DALI_TEST_CHECK(!group.GetSelectedMember());
  END_TEST;
}

int UtcDaliSelectableGroupClearSelectionEmptyNotAllowedP(void)
{
  UiTestApplication application;
  SelectableGroup   group = SelectableGroup::New();
  View              a     = CreateView(application);
  group.Add(a);
  group.SetAllowEmptySelection(false);

  // The group has never had a selection: clearing an already-empty group always
  // succeeds, even when empty selection is disallowed.
  DALI_TEST_CHECK(group.ClearSelection());
  DALI_TEST_CHECK(!group.GetSelectedMember());
  END_TEST;
}

int UtcDaliSelectableGroupAddPreservesNonDefaultRoleP(void)
{
  UiTestApplication application;
  SelectableGroup   group = SelectableGroup::New();
  View              a     = CreateView(application);

  // Host sets a non-default role before joining the group.
  a.SetProperty(View::Property::ACCESSIBILITY_ROLE, AccessibilityRole::BUTTON);
  group.Add(a);

  // The host's role is preserved (not clobbered with RADIO_BUTTON)...
  DALI_TEST_EQUALS(a.GetProperty<int>(View::Property::ACCESSIBILITY_ROLE),
                   static_cast<int>(AccessibilityRole::BUTTON),
                   TEST_LOCATION);

  // ...and restored on Remove.
  group.Remove(a);
  DALI_TEST_EQUALS(a.GetProperty<int>(View::Property::ACCESSIBILITY_ROLE),
                   static_cast<int>(AccessibilityRole::BUTTON),
                   TEST_LOCATION);
  END_TEST;
}

int UtcDaliSelectableGroupReAddSameViewRestoresRadioCostumeP(void)
{
  UiTestApplication application;
  SelectableGroup   group = SelectableGroup::New();
  View              view  = CreateView(application);

  group.Add(view);
  group.SelectMember(view);
  application.SendNotification();
  application.Render();
  DALI_TEST_CHECK(IsSelected(view));
  DALI_TEST_CHECK(IsChecked(view));
  DALI_TEST_EQUALS(view.GetProperty<int>(View::Property::ACCESSIBILITY_ROLE),
                   static_cast<int>(AccessibilityRole::RADIO_BUTTON),
                   TEST_LOCATION);

  // Remove undoes the radio costume but preserves the selected bool.
  DALI_TEST_CHECK(group.Remove(view));
  DALI_TEST_CHECK(IsSelected(view));
  DALI_TEST_CHECK(!IsChecked(view));
  DALI_TEST_EQUALS(view.GetProperty<int>(View::Property::ACCESSIBILITY_ROLE),
                   static_cast<int>(AccessibilityRole::NONE),
                   TEST_LOCATION);

  // Re-Add the same (still-selected) View: it becomes the winner of the empty group
  // and re-acquires RADIO_BUTTON + CHECKED. This is the F1 regression guard.
  DALI_TEST_CHECK(group.Add(view));
  DALI_TEST_CHECK(group.GetSelectedMember() == view);
  DALI_TEST_CHECK(IsSelected(view));
  DALI_TEST_CHECK(IsChecked(view));
  DALI_TEST_EQUALS(view.GetProperty<int>(View::Property::ACCESSIBILITY_ROLE),
                   static_cast<int>(AccessibilityRole::RADIO_BUTTON),
                   TEST_LOCATION);
  END_TEST;
}

int UtcDaliSelectableGroupRoleRecapturedAcrossCycleP(void)
{
  UiTestApplication application;
  SelectableGroup   group = SelectableGroup::New();
  View              view  = CreateView(application);

  // First Add: role NONE -> RADIO_BUTTON, mPreviousRole captured NONE.
  group.Add(view);
  DALI_TEST_EQUALS(view.GetProperty<int>(View::Property::ACCESSIBILITY_ROLE),
                   static_cast<int>(AccessibilityRole::RADIO_BUTTON),
                   TEST_LOCATION);

  // Remove restores NONE.
  group.Remove(view);
  DALI_TEST_EQUALS(view.GetProperty<int>(View::Property::ACCESSIBILITY_ROLE),
                   static_cast<int>(AccessibilityRole::NONE),
                   TEST_LOCATION);

  // The app changes the role between cycles, then re-Adds. The non-NONE role is
  // preserved, and the role is re-captured (BUTTON) rather than frozen at NONE.
  view.SetProperty(View::Property::ACCESSIBILITY_ROLE, AccessibilityRole::BUTTON);
  group.Add(view);
  DALI_TEST_EQUALS(view.GetProperty<int>(View::Property::ACCESSIBILITY_ROLE),
                   static_cast<int>(AccessibilityRole::BUTTON),
                   TEST_LOCATION);

  // Remove restores the live pre-Add role (BUTTON), proving mPreviousRole is not
  // frozen at the first-attach NONE. This is the F2 regression guard.
  group.Remove(view);
  DALI_TEST_EQUALS(view.GetProperty<int>(View::Property::ACCESSIBILITY_ROLE),
                   static_cast<int>(AccessibilityRole::BUTTON),
                   TEST_LOCATION);
  END_TEST;
}

int UtcDaliSelectableGroupRebindSelectedIntoEmptyGroupReappliesCostumeP(void)
{
  UiTestApplication application;
  SelectableGroup   groupA = SelectableGroup::New();
  SelectableGroup   groupB = SelectableGroup::New();
  View              a      = CreateView(application);

  groupA.Add(a);
  groupA.SelectMember(a);
  DALI_TEST_CHECK(IsChecked(a));

  // Cross-group rebind of the already-selected member into the empty group B: a
  // becomes B's winner and re-acquires RADIO_BUTTON + CHECKED via the rebind branch.
  DALI_TEST_CHECK(groupB.Add(a));
  DALI_TEST_CHECK(groupB.GetSelectedMember() == a);
  DALI_TEST_CHECK(a.AsSelectable().IsSelected());
  DALI_TEST_CHECK(IsChecked(a));
  DALI_TEST_EQUALS(a.GetProperty<int>(View::Property::ACCESSIBILITY_ROLE),
                   static_cast<int>(AccessibilityRole::RADIO_BUTTON),
                   TEST_LOCATION);

  // Continue to a Remove: the cross-group rebind must NOT have clobbered the original
  // NONE role (the live role at rebind was still group A's RADIO_BUTTON). Remove must
  // restore NONE and clear CHECKED. This closes the N1 gap: the immediate-after-rebind
  // checks above passed even while N1 was present.
  DALI_TEST_CHECK(groupB.Remove(a));
  DALI_TEST_CHECK(!IsChecked(a));
  DALI_TEST_EQUALS(a.GetProperty<int>(View::Property::ACCESSIBILITY_ROLE),
                   static_cast<int>(AccessibilityRole::NONE),
                   TEST_LOCATION);
  END_TEST;
}

int UtcDaliSelectableGroupExplicitNoneRoleReplacedP(void)
{
  UiTestApplication application;
  SelectableGroup   group = SelectableGroup::New();
  View              view  = CreateView(application);

  // An explicit NONE before Add() is indistinguishable from the default and is
  // replaced with RADIO_BUTTON (F3 documented contract).
  view.SetProperty(View::Property::ACCESSIBILITY_ROLE, AccessibilityRole::NONE);
  group.Add(view);
  DALI_TEST_EQUALS(view.GetProperty<int>(View::Property::ACCESSIBILITY_ROLE),
                   static_cast<int>(AccessibilityRole::RADIO_BUTTON),
                   TEST_LOCATION);
  END_TEST;
}

int UtcDaliSelectableGroupReAddSameGroupKeepsPreviousRoleP(void)
{
  UiTestApplication application;
  SelectableGroup   group = SelectableGroup::New();
  View              view  = CreateView(application);

  // First Add: role NONE -> RADIO_BUTTON, mPreviousRole captured NONE.
  group.Add(view);
  DALI_TEST_EQUALS(view.GetProperty<int>(View::Property::ACCESSIBILITY_ROLE),
                   static_cast<int>(AccessibilityRole::RADIO_BUTTON),
                   TEST_LOCATION);

  // A redundant Add of a View already in this group is a no-op: it must NOT re-capture
  // mPreviousRole as the current RADIO_BUTTON, and must not duplicate the membership.
  DALI_TEST_CHECK(group.Add(view));
  DALI_TEST_EQUALS(group.GetMemberCount(), 1u, TEST_LOCATION);

  // Remove therefore restores the true pre-Add role (NONE), not the leftover RADIO_BUTTON.
  group.Remove(view);
  DALI_TEST_EQUALS(view.GetProperty<int>(View::Property::ACCESSIBILITY_ROLE),
                   static_cast<int>(AccessibilityRole::NONE),
                   TEST_LOCATION);
  END_TEST;
}

int UtcDaliSelectableGroupCrossGroupRebindThenRemoveRestoresOriginalRoleP(void)
{
  UiTestApplication application;
  SelectableGroup   groupA = SelectableGroup::New();
  SelectableGroup   groupB = SelectableGroup::New();
  View              v      = CreateView(application);

  // Default-role (NONE) View joins group A: NONE -> RADIO_BUTTON, mPreviousRole = NONE.
  groupA.Add(v);
  DALI_TEST_EQUALS(v.GetProperty<int>(View::Property::ACCESSIBILITY_ROLE),
                   static_cast<int>(AccessibilityRole::RADIO_BUTTON),
                   TEST_LOCATION);

  // Cross-group rebind into group B: there is NO intervening Remove, so the live role is
  // still group A's RADIO_BUTTON. The capture-guard must skip capturing it, preserving the
  // true original NONE in mPreviousRole.
  groupB.Add(v);

  // Remove from group B must restore the original NONE, not the leftover RADIO_BUTTON.
  // (Before the N1 fix this restored RADIO_BUTTON (221) instead of NONE (216).)
  DALI_TEST_CHECK(groupB.Remove(v));
  DALI_TEST_EQUALS(v.GetProperty<int>(View::Property::ACCESSIBILITY_ROLE),
                   static_cast<int>(AccessibilityRole::NONE),
                   TEST_LOCATION);
  END_TEST;
}

int UtcDaliSelectableGroupCrossGroupRebindSelectedThenRemoveRestoresRoleP(void)
{
  UiTestApplication application;
  SelectableGroup   groupA = SelectableGroup::New();
  SelectableGroup   groupB = SelectableGroup::New();
  View              v      = CreateView(application);

  // Default-role (NONE) View joins group A and is selected: RADIO_BUTTON + CHECKED.
  groupA.Add(v);
  groupA.SelectMember(v);
  application.SendNotification();
  application.Render();
  DALI_TEST_CHECK(IsChecked(v));

  // Cross-group rebind into the empty group B (no intervening Remove): v becomes B's winner.
  groupB.Add(v);

  // Remove from group B restores the original NONE and clears CHECKED, even though the
  // member was selected across the rebind (the CHECKED clear and role restore both fire).
  DALI_TEST_CHECK(groupB.Remove(v));
  DALI_TEST_EQUALS(v.GetProperty<int>(View::Property::ACCESSIBILITY_ROLE),
                   static_cast<int>(AccessibilityRole::NONE),
                   TEST_LOCATION);
  DALI_TEST_CHECK(!IsChecked(v));
  END_TEST;
}

int UtcDaliSelectableGroupCrossGroupRebindNonDefaultRoleSurvivesRoundTripP(void)
{
  UiTestApplication application;
  SelectableGroup   groupA = SelectableGroup::New();
  SelectableGroup   groupB = SelectableGroup::New();
  View              v      = CreateView(application);

  // A non-NONE original role is never overwritten with RADIO_BUTTON, so the live role on
  // rebind is BUTTON (not our costume); the capture-guard is a no-op for this case and the
  // round-trip is unaffected.
  v.SetProperty(View::Property::ACCESSIBILITY_ROLE, AccessibilityRole::BUTTON);
  groupA.Add(v);
  DALI_TEST_EQUALS(v.GetProperty<int>(View::Property::ACCESSIBILITY_ROLE),
                   static_cast<int>(AccessibilityRole::BUTTON),
                   TEST_LOCATION);

  groupB.Add(v);
  DALI_TEST_EQUALS(v.GetProperty<int>(View::Property::ACCESSIBILITY_ROLE),
                   static_cast<int>(AccessibilityRole::BUTTON),
                   TEST_LOCATION);

  // Remove restores the original BUTTON.
  DALI_TEST_CHECK(groupB.Remove(v));
  DALI_TEST_EQUALS(v.GetProperty<int>(View::Property::ACCESSIBILITY_ROLE),
                   static_cast<int>(AccessibilityRole::BUTTON),
                   TEST_LOCATION);
  END_TEST;
}
