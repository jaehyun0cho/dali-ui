# group-selectable-trait sample

Demonstrates single-selection (radio-button style) grouping with
`GroupSelectableTrait` and an explicit `SelectionGroup`, using plain circular
Views stacked vertically with a status label on top.

- Four options are ordinary `View`s made round with a relative corner radius
  (`SetCornerRadiusPolicyRelative()` + `SetCornerRadius(0.5f)`).
- A single controller is created with `SelectionGroup::New()`. Each option is
  bound to it with `SelectionGroup::Add(view)`. `Add()` makes the View
  group-selectable (`GroupSelectableTrait` composes a `SelectableTrait`, which
  implies an `InteractiveTrait`) and installs the group's select-only click
  wiring plus the `RADIO_BUTTON` accessibility costume.
- Tapping an option selects it and deselects the previously selected one;
  re-tapping the selected option is a no-op (true radio semantics), and a gesture
  can never empty the group.
- `SelectedMemberChangedSignal(previous, current, event)` recolours the previous
  option grey and the current option blue, and updates the top `Label`
  (e.g. "1st View is selected"; empty -> "No View is selected").

## Programmatic control

The current `SelectionGroup` API has no group-level "select" setter; programmatic
selection goes through the member's `SelectableTrait`, and the group is emptied
explicitly with `ClearSelection()`:

- `view.AsSelectable().SetSelected(true)` selects a member (the group observes the
  change and enforces single selection).
- `SelectionGroup::ClearSelection()` empties the group.
- `SelectionGroup::GetSelectedMember()` returns the current winner (or an empty
  handle).

## Keys

- `1` .. `4` : select that option programmatically (via its `SelectableTrait`)
- `c`        : clear the selection (`SelectionGroup::ClearSelection`)
- `Escape` / `Back` : quit

`SelectionGroup` is the backing single-selection controller, not a visual
radio-button component; the colour/shape here is just sample styling.
