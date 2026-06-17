# SelectableGroup and GroupSelectableTrait

`SelectableGroup` is a logical controller that enforces **single selection**
(mutual exclusion) across a set of selectable member Views. It is the backing
infrastructure for radio-button-style behaviour: at most one member of a group
is selected at a time, and selecting a member automatically deselects the
previous winner.

A group is **not** part of the View hierarchy. Members join through `Add()` and
are tracked weakly, so a group never extends the lifetime of its members. Each
member is a View backed by a `GroupSelectableTrait` (a `SelectableTrait`
subclass that shares the same reserved selectable trait slot), which `Add()`
attaches automatically.

## Class roles

| Type | Kind | Responsibility |
|---|---|---|
| `SelectableGroup` | public `BaseHandle` | Group controller: membership, selection, signal |
| `GroupSelectableTrait` | public `SelectableTrait` subclass | Per-member trait whose selection is arbitrated by the group |
| `SelectableGroupImpl` | `BaseObject` (internal) | Single-selection state machine |
| `GroupSelectableTraitImpl` | `SelectableTraitImpl` subclass (internal) | Routes selection through the group; accessibility sync |

Ownership: a member trait holds a **strong** reference to its group, while the
group holds **weak** references to its members. There is therefore no reference
cycle, and the group keeps working (for the remaining members) even after the
application drops its `SelectableGroup` handle.

## Basic usage

```cpp
using namespace Dali::Ui;

SelectableGroup group = SelectableGroup::New();

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

- No selectable trait present → a `GroupSelectableTrait` is created and attached.
- A `GroupSelectableTrait` present (possibly from another group) → rebound to
  this group.
- A plain `SelectableTrait` present → **not** replaced; `Add()` returns `false`
  (this avoids losing existing selected state and signal subscribers).

A plain `SelectableTrait` is never replaced, so **add a View to its group before
calling `View::AsSelectable()` on it**; the reverse order fails (and logs an error).

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
navigation across a group, a `View::AsGroupSelectable()` convenience, in-place
upgrade of a plain `SelectableTrait`, and a mandatory-selection (non-empty) mode
are intentionally out of scope here and can be layered on top without disturbing
this design.
