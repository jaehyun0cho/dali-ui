# SelectionGroup

`SelectionGroup` is a logical controller that enforces **single selection**
(mutual exclusion) across a set of selectable member Views. It is the backing
infrastructure for radio-button-style behaviour: at most one member of a group
is selected at a time, and selecting a member automatically deselects the
previous winner.

A group is **not** part of the View hierarchy. Members join through `Add()` and
are tracked weakly, so a group never extends the lifetime of its members. Each
member is a View backed by a `SelectableTrait` that `Add()` binds to the group:
the trait is the same one returned by `View::AsSelectable()`, with its
impl additionally bound to a group so its selection is arbitrated for mutual
exclusion. A member's group can be queried via `SelectableTrait::GetGroup()`
(an empty handle means the trait is not bound to any group).

## Class roles

| Type | Kind | Responsibility |
|---|---|---|
| `SelectionGroup` | public `BaseHandle` | Group controller: membership, selection, signal |
| `SelectableTrait` | public `BaseHandle` | Per-member trait; selection is arbitrated by its group while bound |
| `GroupSelectableTrait` | public `BaseHandle` | Declarative grouping trait in its own slot; composes the SelectableTrait and orchestrates membership |
| `SelectionGroupImpl` | `BaseObject` (internal) | Single-selection state machine |
| `SelectableTraitImpl` | `TraitObject` (internal) | Selection state; routes through the group and syncs accessibility while bound |
| `GroupSelectableTraitImpl` | `TraitObject` (internal) | Declarative grouping orchestration over `SelectionGroup::Find/Add/Remove` |

Ownership: a member trait holds a **strong** reference to its group, while the
group holds **weak** references to its members. There is therefore no reference
cycle, and the group keeps working (for the remaining members) even after the
application drops its `SelectionGroup` handle.

## Basic usage

```cpp
using namespace Dali::Ui;

SelectionGroup group = SelectionGroup::New();

View option1 = View::New();
View option2 = View::New();
View option3 = View::New();

group.Add(option1);
group.Add(option2);
group.Add(option3);

group.SelectedMemberChangedSignal().Connect(
  &myObject, [](View previous, View current, InputEvent event) {
    // previous/current may be empty Views ("no selection").
  });

group.SelectMember(option2); // option2 selected; any previous winner deselected
```

With toggle-by-click enabled (the default, inherited from `SelectableTrait`),
tapping a member selects it and deselects the previous winner. Tapping the
member that is already selected is a no-op.

## Selection rules

- **Mutual exclusion.** Selecting a member deselects the previous winner.
- **Re-selecting the winner is a no-op.** Re-clicking the selected member, or
  calling `SetSelected(false)` on it, does nothing. The only way to leave a
  group with no selection is `ClearSelection()`.
- **Empty selection.** Allowed by default (`SetAllowEmptySelection`). This only
  governs whether `ClearSelection()` may empty the group; it never lets a user
  empty the group by clicking (the winner re-click is always a no-op). When
  disabled, `ClearSelection()` fails on a group that has a winner and the winner
  stays selected; clearing an already-empty group always succeeds regardless.
- **Adding an already-selected member never steals selection.** If a group
  already has a winner, adding another already-selected member keeps the
  existing winner and deselects the newcomer.

## Signal ordering

A swap from member `A` to member `B` produces, in order:

1. `A` item `SelectionChangedSignal(false)`  + `A` accessibility `CHECKED` cleared
2. `B` item `SelectionChangedSignal(true)`   + `B` accessibility `CHECKED` set
3. group `SelectedMemberChangedSignal(A, B, event)` — emitted exactly once

`ClearSelection()` produces: winner item `SelectionChangedSignal(false)` then
group `SelectedMemberChangedSignal(winner, empty, None)`.

The originating `InputEvent` flows through to the group signal: it is the event
delivered to the winner's commit (e.g. the tap that selected it), or
`InputEvent::None()` for programmatic changes and clears.

> The group itself is not passed to the callback. A handler is connected to a
> specific group's signal, so it already has that context. (`Dali::Signal`
> supports at most three arguments.)

## Membership semantics

`Add(View)`:

- No selectable trait present → a `SelectableTrait` is created, attached, and
  bound to this group.
- A `SelectableTrait` present → that existing trait is **bound in place** to this
  group. Its selected state and its signal subscribers are preserved; only the
  group binding and the radio costume are added.
- A `SelectableTrait` already bound to another group → rebound to this group.

Membership is therefore **order-independent**: a View may be made selectable
(via `View::AsSelectable()`) either before or after it is added to a group, with
the same result. The existing trait object is never replaced, so no selected
state and no signal subscribers are ever lost.

`Remove(View)` removes the group membership and undoes the radio accessibility:
the member's selected state is preserved, but its `CHECKED` bit is cleared and its
`ACCESSIBILITY_ROLE` is restored to the role that was live just before this `Add()`
(re-captured on each `Add()`/rebind, not frozen at first attach). The View becomes
an ordinary standalone selectable. The round-trip is symmetric: a re-`Add()` of a
removed member, and a cross-group rebind of the View, re-apply the radio costume
(`RADIO_BUTTON` re-applied when the role is the default `NONE`, and `CHECKED`
re-seeded from the member's preserved selected bool).

Both `Add` and `Remove` (and `SelectMember`/`ClearSelection`) return `false`
while a selection transition is in progress (re-entrancy guard).

Membership operations do **not** emit `SelectedMemberChangedSignal`, even when they
change which member the group reports as selected (first-selected `Add`, winner
`Remove`, cross-group rebind). That signal is reserved for selection changes
(`SelectMember`/click/`ClearSelection`); query `GetSelectedMember()` after a
membership change if you need the post-operation state.

## Finding a group

Besides `New()`, a group can be looked up with **implicit creation on first
lookup**:

```cpp
SelectionGroup byName   = SelectionGroup::Find("colours");   // by name
SelectionGroup byParent = SelectionGroup::Find(parentView);  // by parent View
```

- The first `Find()` for a given key creates a new group and registers it under
  that key; later `Find()` calls for the same key return the **same** group while
  it is still alive.
- Both registries are **weak**: they never extend a group's lifetime. Once every
  `SelectionGroup` handle and every member keeping a group alive is dropped, the
  entry is purged, and a subsequent `Find()` of the same key creates a fresh
  group. (A member holds a strong reference to its group, so adding members keeps
  a group alive even after the application drops its handle.)
- A freshly created group has **`GetMemberCount() == 0`** and allows empty
  selection. To detect a first lookup, check `GetMemberCount() == 0` right after
  `Find()` (the "count == 0" idiom) and populate it if so.
- `Find(parentView)` only provides a stable per-parent group handle; it does not
  add the parent or its children to the group.

## Declarative grouping (`GroupSelectableTrait`)

There are two layers for placing a View into a group:

1. **Imperative** — `SelectionGroup::Add(view)` / `Remove(view)`. The low-level
   path: the caller decides which group a View belongs to and when. This is the
   foundation the next layer is built on.
2. **Declarative** — `View::AsGroupSelectable()` attaches a `GroupSelectableTrait`
   that expresses *intent* ("this View belongs to a single-selection group") and
   resolves membership for you through `SelectionGroup::Find/Add/Remove`.

`GroupSelectableTrait` is **not** a `SelectableTrait` and does not replace one. It
lives in its own reserved trait slot and **composes** the View's `SelectableTrait`:
attaching it ensures a `SelectableTrait` exists (reusing the existing one, never
swapping it), so the View becomes selectable and the existing mutual-exclusion
arbitration in `SelectableTraitImpl` is reused unchanged. Because it composes
rather than swaps, the order of `AsSelectable()` and `AsGroupSelectable()` does not
matter, and any signal subscribers already attached to the selectable trait are
preserved.

```cpp
using namespace Dali::Ui;

// Parent auto-grouping (the default): each child joins its parent's group.
View parent = View::New();
View childA = View::New();
View childB = View::New();
childA.AsGroupSelectable();
childB.AsGroupSelectable();
parent.Add(childA);
parent.Add(childB);
// On scene connection, childA and childB join SelectionGroup::Find(parent).

// Explicit group name (overrides parent auto-grouping):
View option = View::New();
option.AsGroupSelectable().SetGroupName("colors"); // joins SelectionGroup::Find("colors")
```

### Precedence and defaults

- **Parent auto-grouping is on by default.** When no explicit group name is set,
  the View joins `SelectionGroup::Find(parentView)` on scene connection, and leaves
  it on scene disconnection (parent-auto membership is **scene-scoped**). Reparenting
  therefore moves the View from its old parent's group to the new parent's group.
- **An explicit group name always wins.** `SetGroupName(name)` joins
  `SelectionGroup::Find(name)` eagerly and suppresses parent auto-grouping. Setting
  an empty name clears the explicit name and restores parent auto-grouping on the
  next scene connection.
- A plain selectable View (one made selectable with `View::AsSelectable()` but
  **without** `AsGroupSelectable()`) is never auto-grouped — it stays an independent
  checkbox-style selectable even under a grouping parent.

### Querying membership

`GroupSelectableTrait::GetGroup()` reports the View's current group: the named
group when an explicit name is set, otherwise the group the View's
`SelectableTrait` is bound to (its parent's group for an auto-grouped member, or an
empty handle when it is not in any group). `GetGroupName()` returns the explicit
name, or an empty string when none is set.

## Accessibility

Each member whose `ACCESSIBILITY_ROLE` is the default `NONE` is given the
`RADIO_BUTTON` role; a non-`NONE` role the application set before `Add()` is preserved
(and restored on `Remove()`). An explicit `NONE` is indistinguishable from the default
and is therefore replaced with `RADIO_BUTTON`. Likewise, a `RADIO_BUTTON` the application
sets before `Add()` is indistinguishable from the group's own costume, so `Remove()`
restores `NONE` rather than the app's `RADIO_BUTTON` (same nature as the explicit-`NONE`
limitation). The captured role is re-captured on each
`Add()`/rebind, so a `Remove()` restores the role live just before that bind — and the
radio costume is re-applied on re-`Add()`/rebind, so the lock-step holds for every path.
The accessibility `CHECKED` bit is kept in lock-step with the committed selection
state for both the winner (set) and the loser (cleared), using a read-modify-write
so the other state bits (e.g. `ENABLED`) are preserved. `ViewState::SELECTED` is managed by
the base selectable commit path; the `CHECKED` bit is a separate accessibility
state required for radio announcements.

## Lifecycle and re-entrancy

The state machine uses a persistent transition state plus a dismiss-on-success
scope guard, so a selection swap is exception-safe and survives the cross-call
sequence (group arbitration → base winner commit → winner post-commit flush).
If a member detaches or its View is destroyed mid-transition, the group cancels
the pending transition and clears its selection if that member was the winner —
there is no automatic promotion and no group signal during cleanup.

During a swap the prospective winner is recorded before the previous winner is
deselected (so the cancel-on-detach path can recognise it). Consequently
`GetSelectedMember()`, if queried from within the loser's deselect handler, returns
the prospective new winner rather than the old one. Outside a transition it always
returns the committed winner.

## Scope

This is backing infrastructure only. A visual radio-button component, keyboard
navigation across a group, and a mandatory-selection (non-empty) mode are
intentionally out of scope here and can be layered on top without disturbing
this design.
