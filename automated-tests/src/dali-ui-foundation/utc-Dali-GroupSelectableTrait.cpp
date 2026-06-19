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
#include <string>

#include <dali-ui-foundation/dali-ui-foundation.h>
#include <dali-ui-foundation/public-api/group-selectable-trait.h>
#include <dali-ui-foundation/public-api/selectable-trait.h>
#include <dali-ui-foundation/public-api/selection-group.h>
#include <dali-ui-foundation/public-api/view-accessibility-enums.h>
#include <dali-ui-test-suite-utils.h>
#include <dali.h>

using namespace Dali;
using namespace Dali::Ui;

namespace
{

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

// Creates a parent View on the scene and pushes the connection through.
View CreateSceneParent(UiTestApplication& application)
{
  View parent = View::New();
  parent.SetRequestedWidth(400.0f);
  parent.SetRequestedHeight(400.0f);
  application.GetScene().Add(parent);
  application.SendNotification();
  application.Render();
  return parent;
}

bool IsRadioButton(View view)
{
  return view.GetProperty<int>(View::Property::ACCESSIBILITY_ROLE) ==
         static_cast<int>(AccessibilityRole::RADIO_BUTTON);
}

// Records whether a SelectableTrait selection-changed handler fired. Inherits
// ConnectionTracker so it can be used as the connection tracker for the signal.
struct SelectionRecorder : public ConnectionTracker
{
  void OnSelectionChanged(View, bool selected, InputEvent)
  {
    fired        = true;
    lastSelected = selected;
  }
  bool fired        = false;
  bool lastSelected = false;
};

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

int UtcDaliGroupSelectableTraitNewP(void)
{
  UiTestApplication    application;
  GroupSelectableTrait trait = GroupSelectableTrait::New();
  DALI_TEST_CHECK(trait);
  DALI_TEST_CHECK(trait.GetGroupName().empty());
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

// ============================================================================
// Composition with SelectableTrait
// ============================================================================

// AsGroupSelectable ensures the View also becomes selectable (composes, not swaps).
int UtcDaliGroupSelectableTraitComposesSelectableP(void)
{
  UiTestApplication application;
  View              view = CreateView();

  DALI_TEST_CHECK(!view.IsSelectable());
  view.AsGroupSelectable();
  DALI_TEST_CHECK(view.IsSelectable());
  END_TEST;
}

// AsGroupSelectable is idempotent: same trait object returned each time.
int UtcDaliGroupSelectableTraitReuseExistingTraitP(void)
{
  UiTestApplication application;
  View              view = CreateView();

  GroupSelectableTrait first  = view.AsGroupSelectable();
  GroupSelectableTrait second = view.AsGroupSelectable();
  DALI_TEST_CHECK(first == second);
  END_TEST;
}

// Order-independence: AsSelectable then AsGroupSelectable (and the reverse) yield the
// same single SelectableTrait object on the View.
int UtcDaliGroupSelectableTraitOrderIndependenceP(void)
{
  // Order 1: AsSelectable first, then AsGroupSelectable.
  {
    UiTestApplication application;
    View              view       = CreateView();
    SelectableTrait   selectable = view.AsSelectable();
    view.AsGroupSelectable();
    DALI_TEST_CHECK(view.AsSelectable() == selectable);
  }

  // Order 2: AsGroupSelectable first, then AsSelectable.
  {
    UiTestApplication application;
    View              view = CreateView();
    view.AsGroupSelectable();
    SelectableTrait selectable = view.AsSelectable();
    DALI_TEST_CHECK(view.AsSelectable() == selectable);
  }
  END_TEST;
}

// Reusing the existing SelectableTrait preserves its subscribers and identity.
int UtcDaliGroupSelectableTraitPreservesSubscribersP(void)
{
  UiTestApplication application;
  View              view       = CreateView();
  SelectableTrait   selectable = view.AsSelectable();

  SelectionRecorder recorder;
  selectable.SelectionChangedSignal().Connect(&recorder, &SelectionRecorder::OnSelectionChanged);

  // Adding the group trait must NOT swap the selectable: same object identity.
  view.AsGroupSelectable();
  DALI_TEST_CHECK(view.AsSelectable() == selectable);

  // The pre-existing subscriber still fires.
  selectable.SetSelected(true);
  DALI_TEST_CHECK(recorder.fired);
  DALI_TEST_CHECK(recorder.lastSelected);
  END_TEST;
}

// ============================================================================
// Explicit group name
// ============================================================================

int UtcDaliGroupSelectableTraitSetGroupNameJoinsP(void)
{
  UiTestApplication    application;
  View                 view  = CreateView();
  GroupSelectableTrait trait = view.AsGroupSelectable();

  trait.SetGroupName("colors");
  DALI_TEST_EQUALS(trait.GetGroupName(), std::string("colors"), TEST_LOCATION);

  SelectionGroup named = SelectionGroup::Find("colors");
  DALI_TEST_EQUALS(named.GetMemberCount(), 1u, TEST_LOCATION);
  DALI_TEST_CHECK(trait.GetGroup() == named);
  DALI_TEST_CHECK(view.AsSelectable().GetGroup() == named);
  END_TEST;
}

// Two Views sharing an explicit GroupName are mutually exclusive (count == 2).
int UtcDaliGroupSelectableTraitSameGroupNameMutualExclusionP(void)
{
  UiTestApplication application;
  View              a = CreateView();
  View              b = CreateView();

  a.AsGroupSelectable().SetGroupName("g");
  b.AsGroupSelectable().SetGroupName("g");

  SelectionGroup group = SelectionGroup::Find("g");
  DALI_TEST_EQUALS(group.GetMemberCount(), 2u, TEST_LOCATION);

  a.AsSelectable().SetSelected(true);
  DALI_TEST_CHECK(a.AsSelectable().IsSelected());

  // Selecting b deselects a (mutual exclusion).
  b.AsSelectable().SetSelected(true);
  DALI_TEST_CHECK(b.AsSelectable().IsSelected());
  DALI_TEST_CHECK(!a.AsSelectable().IsSelected());
  END_TEST;
}

// Changing the group name leaves the old group and joins the new one.
int UtcDaliGroupSelectableTraitChangeGroupNameP(void)
{
  UiTestApplication    application;
  View                 view  = CreateView();
  GroupSelectableTrait trait = view.AsGroupSelectable();

  trait.SetGroupName("first");
  DALI_TEST_EQUALS(SelectionGroup::Find("first").GetMemberCount(), 1u, TEST_LOCATION);

  trait.SetGroupName("second");
  DALI_TEST_EQUALS(SelectionGroup::Find("first").GetMemberCount(), 0u, TEST_LOCATION);
  DALI_TEST_EQUALS(SelectionGroup::Find("second").GetMemberCount(), 1u, TEST_LOCATION);
  END_TEST;
}

// ============================================================================
// Parent auto-grouping
// ============================================================================

// Two children under one parent auto-join the parent's group on scene connection,
// are mutually exclusive, and the group count is 2.
int UtcDaliGroupSelectableTraitParentAutoGroupP(void)
{
  UiTestApplication application;
  View              parent = CreateSceneParent(application);

  View childA = CreateView();
  View childB = CreateView();
  childA.AsGroupSelectable();
  childB.AsGroupSelectable();

  parent.Add(childA);
  parent.Add(childB);
  application.SendNotification();
  application.Render();

  SelectionGroup parentGroup = SelectionGroup::Find(parent);
  DALI_TEST_EQUALS(parentGroup.GetMemberCount(), 2u, TEST_LOCATION);
  DALI_TEST_CHECK(childA.AsGroupSelectable().GetGroup() == parentGroup);
  DALI_TEST_CHECK(childB.AsGroupSelectable().GetGroup() == parentGroup);

  // Mutual exclusion through the auto-resolved parent group.
  childA.AsSelectable().SetSelected(true);
  DALI_TEST_CHECK(childA.AsSelectable().IsSelected());
  childB.AsSelectable().SetSelected(true);
  DALI_TEST_CHECK(childB.AsSelectable().IsSelected());
  DALI_TEST_CHECK(!childA.AsSelectable().IsSelected());
  END_TEST;
}

// A plain checkbox-style View (AsSelectable only, no group trait) under the same
// parent is NOT auto-grouped: it is independent and gets no RADIO_BUTTON role.
int UtcDaliGroupSelectableTraitCheckboxNotGroupedP(void)
{
  UiTestApplication application;
  View              parent = CreateSceneParent(application);

  View grouped  = CreateView();
  View checkbox = CreateView();
  grouped.AsGroupSelectable();
  checkbox.AsSelectable(); // plain selectable, NOT grouped

  parent.Add(grouped);
  parent.Add(checkbox);
  application.SendNotification();
  application.Render();

  SelectionGroup parentGroup = SelectionGroup::Find(parent);
  DALI_TEST_EQUALS(parentGroup.GetMemberCount(), 1u, TEST_LOCATION);

  // The checkbox is independent (no group) and never received the radio costume.
  DALI_TEST_CHECK(!checkbox.AsSelectable().GetGroup());
  DALI_TEST_CHECK(!IsRadioButton(checkbox));

  // It toggles freely without affecting the grouped member.
  grouped.AsSelectable().SetSelected(true);
  checkbox.AsSelectable().SetSelected(true);
  DALI_TEST_CHECK(checkbox.AsSelectable().IsSelected());
  DALI_TEST_CHECK(grouped.AsSelectable().IsSelected());
  END_TEST;
}

// An explicit group name overrides parent auto-grouping: a named child does NOT
// join the parent's group.
int UtcDaliGroupSelectableTraitExplicitNameOverridesParentP(void)
{
  UiTestApplication application;
  View              parent = CreateSceneParent(application);

  View child = CreateView();
  child.AsGroupSelectable().SetGroupName("explicit");

  parent.Add(child);
  application.SendNotification();
  application.Render();

  // Did not auto-join the parent group; joined the named group instead.
  DALI_TEST_EQUALS(SelectionGroup::Find(parent).GetMemberCount(), 0u, TEST_LOCATION);
  DALI_TEST_EQUALS(SelectionGroup::Find("explicit").GetMemberCount(), 1u, TEST_LOCATION);
  DALI_TEST_CHECK(child.AsGroupSelectable().GetGroup() == SelectionGroup::Find("explicit"));
  END_TEST;
}

// Reparenting: a child auto-grouped under parent1 leaves parent1's group and joins
// parent2's group when moved.
int UtcDaliGroupSelectableTraitReparentP(void)
{
  UiTestApplication application;
  View              parent1 = CreateSceneParent(application);
  View              parent2 = CreateSceneParent(application);

  View child = CreateView();
  child.AsGroupSelectable();

  parent1.Add(child);
  application.SendNotification();
  application.Render();
  DALI_TEST_EQUALS(SelectionGroup::Find(parent1).GetMemberCount(), 1u, TEST_LOCATION);

  // Reparent: this disconnects from the scene under parent1 (leave parent1 group)
  // and reconnects under parent2 (join parent2 group).
  parent2.Add(child);
  application.SendNotification();
  application.Render();

  DALI_TEST_EQUALS(SelectionGroup::Find(parent1).GetMemberCount(), 0u, TEST_LOCATION);
  DALI_TEST_EQUALS(SelectionGroup::Find(parent2).GetMemberCount(), 1u, TEST_LOCATION);
  DALI_TEST_CHECK(child.AsGroupSelectable().GetGroup() == SelectionGroup::Find(parent2));
  END_TEST;
}

// Scene-scoped auto membership: disconnecting from the scene leaves the parent group.
int UtcDaliGroupSelectableTraitSceneDisconnectLeavesParentGroupP(void)
{
  UiTestApplication application;
  View              parent = CreateSceneParent(application);

  View child = CreateView();
  child.AsGroupSelectable();
  parent.Add(child);
  application.SendNotification();
  application.Render();
  DALI_TEST_EQUALS(SelectionGroup::Find(parent).GetMemberCount(), 1u, TEST_LOCATION);

  child.Unparent();
  application.SendNotification();
  application.Render();
  DALI_TEST_EQUALS(SelectionGroup::Find(parent).GetMemberCount(), 0u, TEST_LOCATION);
  END_TEST;
}

// Clearing the group name to empty while on-scene immediately re-joins the parent's
// auto-group, without waiting for a scene reconnection.
int UtcDaliGroupSelectableTraitClearNameRejoinsParentP(void)
{
  UiTestApplication application;
  View              parent = CreateSceneParent(application);

  View childA = CreateView();
  View childB = CreateView();
  childA.AsGroupSelectable();
  childB.AsGroupSelectable();

  parent.Add(childA);
  parent.Add(childB);
  application.SendNotification();
  application.Render();

  // Both children auto-joined the parent's group and are mutually exclusive.
  SelectionGroup parentGroup = SelectionGroup::Find(parent);
  DALI_TEST_EQUALS(parentGroup.GetMemberCount(), 2u, TEST_LOCATION);
  childA.AsSelectable().SetSelected(true);
  childB.AsSelectable().SetSelected(true);
  DALI_TEST_CHECK(childB.AsSelectable().IsSelected());
  DALI_TEST_CHECK(!childA.AsSelectable().IsSelected());

  // Giving childA an explicit name pulls it out of the parent group.
  childA.AsGroupSelectable().SetGroupName("x");
  DALI_TEST_EQUALS(SelectionGroup::Find(parent).GetMemberCount(), 1u, TEST_LOCATION);
  DALI_TEST_CHECK(childA.AsGroupSelectable().GetGroup() == SelectionGroup::Find("x"));

  // Clearing the name while still on-scene re-joins the parent group IMMEDIATELY,
  // with no scene reconnection in between.
  childA.AsGroupSelectable().SetGroupName("");
  DALI_TEST_EQUALS(SelectionGroup::Find(parent).GetMemberCount(), 2u, TEST_LOCATION);
  DALI_TEST_CHECK(childA.AsGroupSelectable().GetGroup() == SelectionGroup::Find(parent));

  // Mutual exclusion through the parent group is restored.
  childA.AsSelectable().SetSelected(true);
  DALI_TEST_CHECK(childA.AsSelectable().IsSelected());
  childB.AsSelectable().SetSelected(true);
  DALI_TEST_CHECK(childB.AsSelectable().IsSelected());
  DALI_TEST_CHECK(!childA.AsSelectable().IsSelected());
  END_TEST;
}

// Clearing the group name while OFF-scene does NOT auto-join immediately; the join is
// deferred to the next scene connection (scene-scoped auto-membership semantics).
int UtcDaliGroupSelectableTraitClearNameOffSceneDefersP(void)
{
  UiTestApplication application;
  View              parent = CreateSceneParent(application);

  // child stays off-scene (never added to parent yet) and carries an explicit name.
  View child = CreateView();
  child.AsGroupSelectable().SetGroupName("named");
  DALI_TEST_EQUALS(SelectionGroup::Find("named").GetMemberCount(), 1u, TEST_LOCATION);

  // Clearing the name off-scene: no immediate parent join (parent not even resolved).
  child.AsGroupSelectable().SetGroupName("");
  DALI_TEST_EQUALS(SelectionGroup::Find("named").GetMemberCount(), 0u, TEST_LOCATION);
  DALI_TEST_EQUALS(SelectionGroup::Find(parent).GetMemberCount(), 0u, TEST_LOCATION);

  // On scene connection under the parent, the deferred auto-join happens.
  parent.Add(child);
  application.SendNotification();
  application.Render();
  DALI_TEST_EQUALS(SelectionGroup::Find(parent).GetMemberCount(), 1u, TEST_LOCATION);
  DALI_TEST_CHECK(child.AsGroupSelectable().GetGroup() == SelectionGroup::Find(parent));
  END_TEST;
}
