# group-selectable-trait sample

Demonstrates declarative single-selection (radio-button style) grouping with
`GroupSelectableTrait`, using plain circular Views stacked vertically with a
status label on top.

- Four options are ordinary `View`s made round with a relative corner radius
  (`SetCornerRadiusPolicyRelative()` + `SetCornerRadius(0.5f)`).
- Each option declares its intent with `View::AsGroupSelectable()`. With parent
  auto-grouping (on by default), once added to the shared stack and connected to
  a scene, each option joins the `SelectionGroup` of its parent View. This also
  makes the View selectable (the group trait composes a `SelectableTrait`) with
  toggle-by-click enabled.
- The parent group is obtained with `SelectionGroup::Find(parentView)` to drive
  selection and observe changes.
- Tapping an option selects it and deselects the previously selected one;
  re-tapping the selected option is a no-op (radio semantics).
- `SelectedMemberChangedSignal(previous, current, event)` recolours the previous
  option grey and the current option blue, and updates the top `Label`
  (e.g. "1st View is selected").

`GroupSelectableTrait` is the declarative layer over the imperative
`SelectionGroup::Add/Remove`; the group itself is the backing single-selection
controller, not a visual radio-button component. The colour/shape here is just
sample styling.

Press Escape or Back to quit.
